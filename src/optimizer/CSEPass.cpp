#include "minic/optimizer/CSEPass.hpp"
#include <unordered_map>

namespace minic::optimizer {

bool CSEPass::runOnFunction(ir::Function& fn) {
    bool changed = false;
    for (auto& bb : fn.blocks()) {
        changed |= cseBasicBlock(*bb);
    }
    return changed;
}

bool CSEPass::cseBasicBlock(ir::BasicBlock& bb) {
    bool changed = false;
    // Map of expression string "op lhs rhs" -> earlier VirtualRegister
    std::unordered_map<std::string, std::shared_ptr<ir::VirtualRegister>> availableExprs;
    std::unordered_map<int, std::shared_ptr<ir::VirtualRegister>> vregReplacements;

    for (auto& inst : bb.instructions()) {
        // First, apply any replacements to operands
        if (auto* bin = dynamic_cast<ir::BinaryInst*>(inst.get())) {
            auto lhsVreg = std::dynamic_pointer_cast<ir::VirtualRegister>(bin->lhs());
            if (lhsVreg && vregReplacements.find(lhsVreg->id()) != vregReplacements.end()) {
                bin->setLhs(vregReplacements[lhsVreg->id()]);
                changed = true;
            }
            auto rhsVreg = std::dynamic_pointer_cast<ir::VirtualRegister>(bin->rhs());
            if (rhsVreg && vregReplacements.find(rhsVreg->id()) != vregReplacements.end()) {
                bin->setRhs(vregReplacements[rhsVreg->id()]);
                changed = true;
            }

            // Expression key
            std::string opStr = ir::opcodeToString(bin->opcode());
            std::string lStr = bin->lhs()->toString();
            std::string rStr = bin->rhs()->toString();

            // Canonicalize commutative operations (Add, Mul)
            if (bin->opcode() == ir::Opcode::Add || bin->opcode() == ir::Opcode::Mul) {
                if (lStr > rStr) {
                    std::swap(lStr, rStr);
                }
            }
            std::string key = opStr + " " + lStr + " " + rStr;

            auto it = availableExprs.find(key);
            if (it != availableExprs.end()) {
                // Redundant computation found! Map this destination register to the earlier register
                vregReplacements[bin->dest()->id()] = it->second;
                changed = true;
            } else {
                availableExprs[key] = bin->dest();
            }
        } else if (auto* store = dynamic_cast<ir::StoreInst*>(inst.get())) {
            auto valVreg = std::dynamic_pointer_cast<ir::VirtualRegister>(store->val());
            if (valVreg && vregReplacements.find(valVreg->id()) != vregReplacements.end()) {
                store->setVal(vregReplacements[valVreg->id()]);
                changed = true;
            }
        } else if (auto* ret = dynamic_cast<ir::ReturnInst*>(inst.get())) {
            if (ret->val()) {
                auto valVreg = std::dynamic_pointer_cast<ir::VirtualRegister>(ret->val());
                if (valVreg && vregReplacements.find(valVreg->id()) != vregReplacements.end()) {
                    ret->setVal(vregReplacements[valVreg->id()]);
                    changed = true;
                }
            }
        }
    }

    return changed;
}

} // namespace minic::optimizer
