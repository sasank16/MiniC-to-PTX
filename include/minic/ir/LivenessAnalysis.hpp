#pragma once

#include "minic/ir/Function.hpp"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>

namespace minic::ir {

struct LiveRange {
    int vregId{-1};
    bool isPredicate{false};
    int start{-1}; // Instruction index where defined
    int end{-1};   // Last instruction index where used

    bool overlaps(const LiveRange& other) const {
        return !(end < other.start || other.end < start);
    }
};

struct BlockLiveness {
    std::unordered_set<int> def;
    std::unordered_set<int> use;
    std::unordered_set<int> liveIn;
    std::unordered_set<int> liveOut;
};

class LivenessAnalysis {
public:
    explicit LivenessAnalysis(const Function& fn);

    void run();

    const BlockLiveness& getBlockLiveness(const BasicBlock* bb) const {
        return blockMap_.at(bb);
    }

    const std::vector<LiveRange>& getLiveRanges() const {
        return liveRanges_;
    }

    int getPeakRegisterPressure() const {
        return peakPressure_;
    }

    void printReport(std::ostream& out) const;

private:
    void computeLocalDefUse();
    void computeGlobalDataflow();
    void computeInstructionIntervals();

    const Function& fn_;
    std::unordered_map<const BasicBlock*, BlockLiveness> blockMap_;
    std::vector<LiveRange> liveRanges_;
    int peakPressure_{0};
};

} // namespace minic::ir
