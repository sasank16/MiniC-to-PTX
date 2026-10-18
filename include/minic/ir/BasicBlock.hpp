#pragma once

#include "minic/ir/Instruction.hpp"
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

namespace minic::ir {

class Function;

class BasicBlock {
public:
    BasicBlock(std::string name, Function* parent)
        : name_(std::move(name)), parent_(parent) {}

    const std::string& name() const { return name_; }
    Function* parent() const { return parent_; }

    const std::vector<std::unique_ptr<Instruction>>& instructions() const { return instructions_; }
    std::vector<std::unique_ptr<Instruction>>& instructions() { return instructions_; }

    const std::vector<BasicBlock*>& predecessors() const { return predecessors_; }
    const std::vector<BasicBlock*>& successors() const { return successors_; }

    void addPredecessor(BasicBlock* pred) {
        if (std::find(predecessors_.begin(), predecessors_.end(), pred) == predecessors_.end()) {
            predecessors_.push_back(pred);
        }
    }

    void removePredecessor(BasicBlock* pred) {
        predecessors_.erase(
            std::remove(predecessors_.begin(), predecessors_.end(), pred),
            predecessors_.end());
    }

    void addSuccessor(BasicBlock* succ) {
        if (std::find(successors_.begin(), successors_.end(), succ) == successors_.end()) {
            successors_.push_back(succ);
        }
    }

    void removeSuccessor(BasicBlock* succ) {
        successors_.erase(
            std::remove(successors_.begin(), successors_.end(), succ),
            successors_.end());
    }

    bool isTerminated() const {
        return !instructions_.empty() && instructions_.back()->isTerminator();
    }

    Instruction* getTerminator() const {
        if (isTerminated()) {
            return instructions_.back().get();
        }
        return nullptr;
    }

    void appendInstruction(std::unique_ptr<Instruction> inst) {
        instructions_.push_back(std::move(inst));
    }

    void print(std::ostream& out) const;

private:
    std::string name_;
    Function* parent_;
    std::vector<std::unique_ptr<Instruction>> instructions_;
    std::vector<BasicBlock*> predecessors_;
    std::vector<BasicBlock*> successors_;
};

} // namespace minic::ir
