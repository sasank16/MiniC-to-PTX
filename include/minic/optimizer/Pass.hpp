#pragma once

#include "minic/ir/Module.hpp"
#include "minic/ir/Function.hpp"
#include <string>
#include <memory>

namespace minic::optimizer {

class Pass {
public:
    virtual ~Pass() = default;

    virtual std::string name() const = 0;
    virtual bool runOnFunction(ir::Function& fn) = 0;

    virtual bool runOnModule(ir::Module& mod) {
        bool changed = false;
        for (auto& fn : mod.functions()) {
            changed |= runOnFunction(*fn);
        }
        return changed;
    }
};

} // namespace minic::optimizer
