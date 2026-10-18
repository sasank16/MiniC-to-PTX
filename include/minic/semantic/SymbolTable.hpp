#pragma once

#include "minic/semantic/Symbol.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <optional>

namespace minic::semantic {

class Scope {
public:
    explicit Scope(std::shared_ptr<Scope> parent = nullptr) : parent_(std::move(parent)) {}

    bool declare(const Symbol& sym) {
        if (symbols_.find(sym.name) != symbols_.end()) {
            return false;
        }
        symbols_.emplace(sym.name, sym);
        return true;
    }

    const Symbol* lookup(const std::string& name) const {
        auto it = symbols_.find(name);
        if (it != symbols_.end()) {
            return &it->second;
        }
        if (parent_) {
            return parent_->lookup(name);
        }
        return nullptr;
    }

    const Symbol* lookupCurrent(const std::string& name) const {
        auto it = symbols_.find(name);
        if (it != symbols_.end()) {
            return &it->second;
        }
        return nullptr;
    }

    std::shared_ptr<Scope> parent() const { return parent_; }

private:
    std::unordered_map<std::string, Symbol> symbols_;
    std::shared_ptr<Scope> parent_;
};

class SymbolTable {
public:
    SymbolTable() {
        // Initialize global scope
        currentScope_ = std::make_shared<Scope>(nullptr);
    }

    void enterScope() {
        currentScope_ = std::make_shared<Scope>(currentScope_);
        depth_++;
    }

    void exitScope() {
        if (currentScope_->parent()) {
            currentScope_ = currentScope_->parent();
            depth_--;
        }
    }

    bool declare(const Symbol& sym) {
        return currentScope_->declare(sym);
    }

    const Symbol* lookup(const std::string& name) const {
        return currentScope_->lookup(name);
    }

    const Symbol* lookupCurrent(const std::string& name) const {
        return currentScope_->lookupCurrent(name);
    }

    size_t depth() const { return depth_; }

private:
    std::shared_ptr<Scope> currentScope_;
    size_t depth_{0};
};

} // namespace minic::semantic
