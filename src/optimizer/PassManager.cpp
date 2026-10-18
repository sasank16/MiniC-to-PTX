#include "minic/optimizer/PassManager.hpp"
#include "minic/optimizer/ConstantFoldingPass.hpp"
#include "minic/optimizer/DeadCodeEliminationPass.hpp"
#include "minic/optimizer/CSEPass.hpp"

namespace minic::optimizer {

bool PassManager::run(ir::Module& mod, int maxIterations) {
    bool anyChanged = false;
    for (int iter = 0; iter < maxIterations; ++iter) {
        bool iterChanged = false;
        for (const auto& pass : passes_) {
            iterChanged |= pass->runOnModule(mod);
        }
        anyChanged |= iterChanged;
        if (!iterChanged) {
            break; // Fixpoint reached
        }
    }
    return anyChanged;
}

PassManager PassManager::createDefaultPipeline() {
    PassManager pm;
    // Pass 1: Constant folding and propagation
    pm.addPass(std::make_unique<ConstantFoldingPass>());
    // Pass 2: Common subexpression elimination
    pm.addPass(std::make_unique<CSEPass>());
    // Pass 3: Dead code elimination
    pm.addPass(std::make_unique<DeadCodeEliminationPass>());
    // Second round of folding & DCE to clean up after CSE
    pm.addPass(std::make_unique<ConstantFoldingPass>());
    pm.addPass(std::make_unique<DeadCodeEliminationPass>());
    return pm;
}

} // namespace minic::optimizer
