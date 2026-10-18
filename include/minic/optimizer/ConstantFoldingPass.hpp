#pragma once

#include "minic/optimizer/Pass.hpp"

namespace minic::optimizer {

class ConstantFoldingPass : public Pass {
public:
    std::string name() const override { return "ConstantFoldingAndPropagation"; }
    bool runOnFunction(ir::Function& fn) override;

private:
    bool foldBasicBlock(ir::BasicBlock& bb, ir::Function& fn);
};

} // namespace minic::optimizer
