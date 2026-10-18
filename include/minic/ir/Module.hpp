#pragma once

#include "minic/ir/Function.hpp"
#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace minic::ir {

class Module {
public:
    explicit Module(std::string name) : name_(std::move(name)) {}

    const std::string& name() const { return name_; }

    const std::vector<std::unique_ptr<Function>>& functions() const { return functions_; }
    std::vector<std::unique_ptr<Function>>& functions() { return functions_; }

    Function* createFunction(std::string name, semantic::Type returnType,
                             std::vector<std::shared_ptr<Parameter>> params,
                             bool isKernel = false) {
        auto fn = std::make_unique<Function>(
            std::move(name), returnType, std::move(params), isKernel, this);
        Function* ptr = fn.get();
        functions_.push_back(std::move(fn));
        return ptr;
    }

    Function* getFunction(const std::string& name) const {
        for (const auto& fn : functions_) {
            if (fn->name() == name) {
                return fn.get();
            }
        }
        return nullptr;
    }

    void print(std::ostream& out) const {
        out << "; ModuleID = '" << name_ << "'\n\n";
        for (const auto& fn : functions_) {
            fn->print(out);
            out << "\n";
        }
    }

    void printCFGDot(std::ostream& out) const {
        for (const auto& fn : functions_) {
            fn->printCFGDot(out);
            out << "\n";
        }
    }

private:
    std::string name_;
    std::vector<std::unique_ptr<Function>> functions_;
};

} // namespace minic::ir
