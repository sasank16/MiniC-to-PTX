#include "minic/optimizer/DeadCodeEliminationPass.hpp"
#include <unordered_set>
#include <queue>
#include <algorithm>

namespace minic::optimizer {

bool DeadCodeEliminationPass::runOnFunction(ir::Function& fn) {
    bool changed = false;
    changed |= eliminateUnreachableBlocks(fn);
    changed |= eliminateDeadInstructions(fn);
    return changed;
}

bool DeadCodeEliminationPass::eliminateUnreachableBlocks(ir::Function& fn) {
    if (fn.blocks().empty()) return false;

    // Reachability analysis from entryBlock using BFS
    std::unordered_set<ir::BasicBlock*> reachable;
    std::queue<ir::BasicBlock*> worklist;

    ir::BasicBlock* entry = fn.entryBlock();
    reachable.insert(entry);
    worklist.push(entry);

    while (!worklist.empty()) {
        ir::BasicBlock* curr = worklist.front();
        worklist.pop();

        for (ir::BasicBlock* succ : curr->successors()) {
            if (reachable.insert(succ).second) {
                worklist.push(succ);
            }
        }
    }

    if (reachable.size() == fn.blocks().size()) {
        return false;
    }

    // Clean up CFG edges from unreachable blocks to reachable blocks
    for (auto& bb : fn.blocks()) {
        if (reachable.find(bb.get()) == reachable.end()) {
            for (ir::BasicBlock* succ : bb->successors()) {
                succ->removePredecessor(bb.get());
            }
        }
    }

    // Remove unreachable blocks
    auto it = std::remove_if(
        fn.blocks().begin(), fn.blocks().end(),
        [&reachable](const std::unique_ptr<ir::BasicBlock>& bb) {
            return reachable.find(bb.get()) == reachable.end();
        });

    fn.blocks().erase(it, fn.blocks().end());
    return true;
}

bool DeadCodeEliminationPass::eliminateDeadInstructions(ir::Function& fn) {
    bool anyChanged = false;
    bool changed = true;

    while (changed) {
        changed = false;

        // 1. Gather all variables that are loaded
        std::unordered_set<std::string> loadedVars;
        for (const auto& bb : fn.blocks()) {
            for (const auto& inst : bb->instructions()) {
                if (auto* load = dynamic_cast<ir::LoadInst*>(inst.get())) {
                    loadedVars.insert(load->varName());
                }
            }
        }

        // 2. Eliminate stores and allocas for variables that are never loaded
        for (auto& bb : fn.blocks()) {
            auto& insts = bb->instructions();
            auto it = std::remove_if(
                insts.begin(), insts.end(),
                [&loadedVars, &changed](const std::unique_ptr<ir::Instruction>& inst) {
                    if (auto* store = dynamic_cast<ir::StoreInst*>(inst.get())) {
                        if (loadedVars.find(store->varName()) == loadedVars.end()) {
                            changed = true;
                            return true;
                        }
                    } else if (auto* alloca = dynamic_cast<ir::AllocaInst*>(inst.get())) {
                        if (loadedVars.find(alloca->varName()) == loadedVars.end()) {
                            changed = true;
                            return true;
                        }
                    }
                    return false;
                });
            insts.erase(it, insts.end());
        }

        // 3. Gather all used virtual register IDs
        std::unordered_set<int> usedVRegs;
        for (const auto& bb : fn.blocks()) {
            for (const auto& inst : bb->instructions()) {
                for (const auto& use : inst->getUses()) {
                    auto vreg = std::dynamic_pointer_cast<ir::VirtualRegister>(use);
                    if (vreg) {
                        usedVRegs.insert(vreg->id());
                    }
                }
            }
        }

        // 4. Erase instructions that define an unused register and have no side effects
        for (auto& bb : fn.blocks()) {
            auto& insts = bb->instructions();
            auto it = std::remove_if(
                insts.begin(), insts.end(),
                [&usedVRegs, &changed](const std::unique_ptr<ir::Instruction>& inst) {
                    auto def = inst->getDef();
                    if (!def) return false;

                    // Instructions with side effects cannot be removed
                    if (inst->opcode() == ir::Opcode::Call) return false;
                    if (inst->opcode() == ir::Opcode::Store) return false;
                    if (inst->opcode() == ir::Opcode::Return) return false;

                    // If defined register is never used, remove
                    if (usedVRegs.find(def->id()) == usedVRegs.end()) {
                        changed = true;
                        return true;
                    }
                    return false;
                });

            insts.erase(it, insts.end());
        }

        anyChanged |= changed;
    }

    return anyChanged;
}

} // namespace minic::optimizer
