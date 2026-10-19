#include "minic/codegen/LinearScanAllocator.hpp"
#include <algorithm>

namespace minic::codegen {

LinearScanAllocator::LinearScanAllocator(const ir::LivenessAnalysis& liveness)
    : liveness_(liveness) {}

AllocationResult LinearScanAllocator::allocate() {
    AllocationResult result;

    const auto& ranges = liveness_.getLiveRanges();

    // Process separately for predicate registers and 32-bit scalar registers
    for (bool isPred : {false, true}) {
        std::vector<ir::LiveRange> subset;
        for (const auto& lr : ranges) {
            if (lr.isPredicate == isPred) {
                subset.push_back(lr);
            }
        }

        // Active intervals currently assigned a register: pair (end_point, assigned_reg)
        std::vector<std::pair<int, int>> active;
        std::set<int> freeRegisters;
        int nextFreshReg = 0;

        for (const auto& lr : subset) {
            // 1. Expire old intervals that ended before lr.start
            auto it = active.begin();
            while (it != active.end()) {
                if (it->first < lr.start) {
                    // Return expired register back to the free pool
                    freeRegisters.insert(it->second);
                    it = active.erase(it);
                } else {
                    ++it;
                }
            }

            // 2. Allocate register for lr
            int assigned = 0;
            if (!freeRegisters.empty()) {
                // Reuse an expired register
                assigned = *freeRegisters.begin();
                freeRegisters.erase(freeRegisters.begin());
            } else {
                // Allocate a new register index
                assigned = nextFreshReg++;
            }

            int64_t key = (static_cast<int64_t>(lr.isPredicate ? 1 : 0) << 32) |
                          static_cast<uint32_t>(lr.vregId);
            result.registerMap[key] = assigned;

            // 3. Add to active set
            active.push_back({lr.end, assigned});
            // Keep active sorted by end point
            std::sort(active.begin(), active.end(),
                      [](const auto& a, const auto& b) {
                          return a.first < b.first;
                      });
        }

        if (isPred) {
            result.numAllocatedPred = nextFreshReg;
        } else {
            result.numAllocatedS32 = nextFreshReg;
        }
    }

    return result;
}

} // namespace minic::codegen
