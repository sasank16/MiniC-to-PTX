#pragma once

#include "minic/optimizer/Pass.hpp"
#include <vector>
#include <memory>

namespace minic::optimizer {

class PassManager {
public:
    PassManager() = default;

    void addPass(std::unique_ptr<Pass> pass) {
        passes_.push_back(std::move(pass));
    }

    // Run all registered passes iteratively until fixpoint or maxIterations is reached
    bool run(ir::Module& mod, int maxIterations = 10);

    // Creates standard optimization pipeline (-O1)
    static PassManager createDefaultPipeline();

private:
    std::vector<std::unique_ptr<Pass>> passes_;
};

} // namespace minic::optimizer
