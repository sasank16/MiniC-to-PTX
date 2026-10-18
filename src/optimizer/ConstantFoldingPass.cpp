#include "minic/optimizer/ConstantFoldingPass.hpp"
#include <unordered_map>
#include <unordered_set>

namespace minic::optimizer {

bool ConstantFoldingPass::runOnFunction(ir::Function& fn) {
    bool anyChanged = false;
    for (auto& bb : fn.blocks()) {
        anyChanged |= foldBasicBlock(*bb, fn);
    }
    return anyChanged;
}

static std::shared_ptr<ir::Value> resolveValue(
    std::shared_ptr<ir::Value> val,
    const std::unordered_map<int, int>& vregConstants,
    const std::unordered_map<int, std::shared_ptr<ir::VirtualRegister>>& vregReplacements) {
    if (!val) return nullptr;
    auto vreg = std::dynamic_pointer_cast<ir::VirtualRegister>(val);
    if (vreg) {
        auto itRep = vregReplacements.find(vreg->id());
        if (itRep != vregReplacements.end()) {
            return resolveValue(itRep->second, vregConstants, vregReplacements);
        }
        auto itConst = vregConstants.find(vreg->id());
        if (itConst != vregConstants.end()) {
            return std::make_shared<ir::ConstantInt>(itConst->second);
        }
    }
    return val;
}

bool ConstantFoldingPass::foldBasicBlock(ir::BasicBlock& bb, ir::Function& fn) {
    (void)fn;
    bool changed = false;
    std::unordered_map<int, int> vregConstants;
    std::unordered_map<int, std::shared_ptr<ir::VirtualRegister>> vregReplacements;
    std::unordered_map<std::string, int> varConstants;
    std::unordered_map<std::string, std::shared_ptr<ir::VirtualRegister>> loadedVarRegs;

    // If this block has exactly one predecessor with a single successor, inherit varConstants
    if (bb.predecessors().size() == 1) {
        ir::BasicBlock* pred = bb.predecessors()[0];
        if (pred->successors().size() == 1) {
            // Find stored constants in pred
            for (const auto& inst : pred->instructions()) {
                if (auto* st = dynamic_cast<ir::StoreInst*>(inst.get())) {
                    auto c = std::dynamic_pointer_cast<ir::ConstantInt>(st->val());
                    if (c) {
                        varConstants[st->varName()] = c->value();
                    } else {
                        varConstants.erase(st->varName());
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < bb.instructions().size(); ++i) {
        auto& inst = bb.instructions()[i];

        if (auto* store = dynamic_cast<ir::StoreInst*>(inst.get())) {
            auto resolved = resolveValue(store->val(), vregConstants, vregReplacements);
            if (resolved != store->val()) {
                store->setVal(resolved);
                changed = true;
            }
            auto c = std::dynamic_pointer_cast<ir::ConstantInt>(store->val());
            if (c) {
                varConstants[store->varName()] = c->value();
            } else {
                varConstants.erase(store->varName());
            }
            // Invalidate loaded register cache for this variable
            loadedVarRegs.erase(store->varName());

        } else if (auto* load = dynamic_cast<ir::LoadInst*>(inst.get())) {
            // Check if variable has a known constant
            auto itConst = varConstants.find(load->varName());
            if (itConst != varConstants.end()) {
                vregConstants[load->dest()->id()] = itConst->second;
            } else {
                // Redundant load elimination: if variable was already loaded into a vreg in this block
                auto itLoad = loadedVarRegs.find(load->varName());
                if (itLoad != loadedVarRegs.end()) {
                    vregReplacements[load->dest()->id()] = itLoad->second;
                    changed = true;
                } else {
                    loadedVarRegs[load->varName()] = load->dest();
                }
            }
        } else if (auto* bin = dynamic_cast<ir::BinaryInst*>(inst.get())) {
            auto lhs = resolveValue(bin->lhs(), vregConstants, vregReplacements);
            auto rhs = resolveValue(bin->rhs(), vregConstants, vregReplacements);

            if (lhs != bin->lhs()) {
                bin->setLhs(lhs);
                changed = true;
            }
            if (rhs != bin->rhs()) {
                bin->setRhs(rhs);
                changed = true;
            }

            auto cLhs = std::dynamic_pointer_cast<ir::ConstantInt>(bin->lhs());
            auto cRhs = std::dynamic_pointer_cast<ir::ConstantInt>(bin->rhs());

            if (cLhs && cRhs) {
                int l = cLhs->value();
                int r = cRhs->value();
                int res = 0;
                bool valid = true;

                switch (bin->opcode()) {
                    case ir::Opcode::Add:  res = l + r; break;
                    case ir::Opcode::Sub:  res = l - r; break;
                    case ir::Opcode::Mul:  res = l * r; break;
                    case ir::Opcode::SDiv: if (r != 0) res = l / r; else valid = false; break;
                    case ir::Opcode::SRem: if (r != 0) res = l % r; else valid = false; break;
                    default: valid = false; break;
                }

                if (valid) {
                    vregConstants[bin->dest()->id()] = res;
                    changed = true;
                }
            } else if (cRhs) {
                int r = cRhs->value();
                if (bin->opcode() == ir::Opcode::Mul && r == 0) {
                    vregConstants[bin->dest()->id()] = 0;
                    changed = true;
                }
            } else if (cLhs) {
                int l = cLhs->value();
                if (bin->opcode() == ir::Opcode::Mul && l == 0) {
                    vregConstants[bin->dest()->id()] = 0;
                    changed = true;
                }
            }
        } else if (auto* un = dynamic_cast<ir::UnaryInst*>(inst.get())) {
            auto opVal = resolveValue(un->operand(), vregConstants, vregReplacements);
            if (opVal != un->operand()) {
                un->setOperand(opVal);
                changed = true;
            }
            auto cOp = std::dynamic_pointer_cast<ir::ConstantInt>(un->operand());
            if (cOp) {
                int val = cOp->value();
                int res = (un->opcode() == ir::Opcode::Neg) ? -val : (val == 0 ? 1 : 0);
                vregConstants[un->dest()->id()] = res;
                changed = true;
            }
        } else if (auto* cmp = dynamic_cast<ir::CompareInst*>(inst.get())) {
            auto lhs = resolveValue(cmp->lhs(), vregConstants, vregReplacements);
            auto rhs = resolveValue(cmp->rhs(), vregConstants, vregReplacements);

            if (lhs != cmp->lhs()) {
                cmp->setLhs(lhs);
                changed = true;
            }
            if (rhs != cmp->rhs()) {
                cmp->setRhs(rhs);
                changed = true;
            }

            auto cLhs = std::dynamic_pointer_cast<ir::ConstantInt>(cmp->lhs());
            auto cRhs = std::dynamic_pointer_cast<ir::ConstantInt>(cmp->rhs());

            if (cLhs && cRhs) {
                int l = cLhs->value();
                int r = cRhs->value();
                bool res = false;
                switch (cmp->opcode()) {
                    case ir::Opcode::CmpEq: res = (l == r); break;
                    case ir::Opcode::CmpNe: res = (l != r); break;
                    case ir::Opcode::CmpLt: res = (l < r); break;
                    case ir::Opcode::CmpLe: res = (l <= r); break;
                    case ir::Opcode::CmpGt: res = (l > r); break;
                    case ir::Opcode::CmpGe: res = (l >= r); break;
                    default: break;
                }
                vregConstants[cmp->dest()->id()] = res ? 1 : 0;
                changed = true;
            }
        } else if (auto* ret = dynamic_cast<ir::ReturnInst*>(inst.get())) {
            if (ret->val()) {
                auto resolved = resolveValue(ret->val(), vregConstants, vregReplacements);
                if (resolved != ret->val()) {
                    ret->setVal(resolved);
                    changed = true;
                }
            }
        } else if (auto* brCond = dynamic_cast<ir::BranchCondInst*>(inst.get())) {
            auto it = vregConstants.find(brCond->cond()->id());
            if (it != vregConstants.end()) {
                bool takeTrue = (it->second != 0);
                ir::BasicBlock* chosenTarget = takeTrue ? brCond->trueTarget() : brCond->falseTarget();
                ir::BasicBlock* eliminatedTarget = takeTrue ? brCond->falseTarget() : brCond->trueTarget();

                bb.removeSuccessor(eliminatedTarget);
                eliminatedTarget->removePredecessor(&bb);

                inst = std::make_unique<ir::BranchInst>(chosenTarget);
                changed = true;
            }
        }
    }

    return changed;
}

} // namespace minic::optimizer
