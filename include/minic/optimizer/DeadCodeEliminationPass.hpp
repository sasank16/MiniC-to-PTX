#pragma once

#include "minic/optimizer/Pass.hpp"

namespace minic::optimizer {

class DeadCodeEliminationPass : public Pass {
public:
    std::string name() const override { return "DeadCodeElimination"; }
    bool runOnFunction(ir::Function& fn) override;

private:
    bool eliminateDeadInstructions(ir::Function& fn);
    bool eliminateUnreachableBlocks(ir::Function& fn);
};

} // namespace minic::optimizer
