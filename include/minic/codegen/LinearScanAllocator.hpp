#pragma once

#include "minic/ir/LivenessAnalysis.hpp"
#include <unordered_map>
#include <vector>
#include <set>

namespace minic::codegen {

struct AllocationResult {
    // Map (isPredicate << 32 | vregId) -> compact register index
    std::unordered_map<int64_t, int> registerMap;
    int numAllocatedS32{0};
    int numAllocatedPred{0};
};

class LinearScanAllocator {
public:
    explicit LinearScanAllocator(const ir::LivenessAnalysis& liveness);

    AllocationResult allocate();

private:
    const ir::LivenessAnalysis& liveness_;
};

} // namespace minic::codegen
