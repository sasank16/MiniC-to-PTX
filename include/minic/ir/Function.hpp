#pragma once

#include "minic/ir/BasicBlock.hpp"
#include "minic/ir/Value.hpp"
#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace minic::ir {

class Module;

class Function {
public:
    Function(std::string name, semantic::Type returnType,
             std::vector<std::shared_ptr<Parameter>> params,
             bool isKernel, Module* parent)
        : name_(std::move(name)), returnType_(returnType),
          params_(std::move(params)), isKernel_(isKernel), parent_(parent) {}

    const std::string& name() const { return name_; }
    semantic::Type returnType() const { return returnType_; }
    const std::vector<std::shared_ptr<Parameter>>& params() const { return params_; }
    bool isKernel() const { return isKernel_; }
    Module* parent() const { return parent_; }

    const std::vector<std::unique_ptr<BasicBlock>>& blocks() const { return blocks_; }
    std::vector<std::unique_ptr<BasicBlock>>& blocks() { return blocks_; }

    BasicBlock* entryBlock() const {
        if (blocks_.empty()) return nullptr;
        return blocks_.front().get();
    }

    BasicBlock* createBasicBlock(const std::string& namePrefix = "bb") {
        std::string blockName = namePrefix + std::to_string(blockCounter_++);
        auto bb = std::make_unique<BasicBlock>(blockName, this);
        BasicBlock* ptr = bb.get();
        blocks_.push_back(std::move(bb));
        return ptr;
    }

    std::shared_ptr<VirtualRegister> createRegister(semantic::Type type, bool isPredicate = false) {
        if (isPredicate) {
            return std::make_shared<VirtualRegister>(predicateCounter_++, type, true);
        }
        return std::make_shared<VirtualRegister>(registerCounter_++, type, false);
    }

    int registerCount() const { return registerCounter_; }
    int predicateCount() const { return predicateCounter_; }

    void print(std::ostream& out) const;
    void printCFGDot(std::ostream& out) const;

private:
    std::string name_;
    semantic::Type returnType_;
    std::vector<std::shared_ptr<Parameter>> params_;
    bool isKernel_{false};
    Module* parent_;

    std::vector<std::unique_ptr<BasicBlock>> blocks_;
    int registerCounter_{0};
    int predicateCounter_{0};
    int blockCounter_{0};
};

} // namespace minic::ir
