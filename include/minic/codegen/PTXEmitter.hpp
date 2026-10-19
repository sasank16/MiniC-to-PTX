#pragma once

#include "minic/ir/Module.hpp"
#include "minic/codegen/PTXTarget.hpp"
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace minic::codegen {

class PTXEmitter {
public:
    explicit PTXEmitter(PTXTarget target = PTXTarget())
        : target_(std::move(target)) {}

    void emitModule(const ir::Module& mod, std::ostream& out);

private:
    void emitHeader(std::ostream& out);
    void emitFunction(const ir::Function& fn, std::ostream& out);
    void emitBasicBlock(const ir::BasicBlock& bb, const ir::Function& fn, std::ostream& out);
    void emitInstruction(const ir::Instruction& inst, const ir::Function& fn, std::ostream& out);

    std::string formatOperand(const ir::Value* val);
    std::string getPTXReg(const ir::VirtualRegister* vreg);
    std::string getVarReg(const std::string& varName);

    PTXTarget target_;
    std::unordered_map<int64_t, std::string> vregMap_;
    std::unordered_map<std::string, std::string> varRegMap_;
    std::unordered_set<std::string> declaredLabels_;
    int nextRegId_{0};
    int nextPredId_{0};
    int nextAddrRegId_{0};
    bool currentIsKernel_{false};
};

} // namespace minic::codegen
