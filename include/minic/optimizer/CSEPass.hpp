#pragma once

#include "minic/optimizer/Pass.hpp"

namespace minic::optimizer {

class CSEPass : public Pass {
public:
    std::string name() const override { return "CommonSubexpressionElimination"; }
    bool runOnFunction(ir::Function& fn) override;

private:
    bool cseBasicBlock(ir::BasicBlock& bb);
};

} // namespace minic::optimizer
