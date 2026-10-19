#include "minic/ir/LivenessAnalysis.hpp"
#include <algorithm>
#include <iomanip>

namespace minic::ir {

LivenessAnalysis::LivenessAnalysis(const Function& fn) : fn_(fn) {}

void LivenessAnalysis::run() {
    computeLocalDefUse();
    computeGlobalDataflow();
    computeInstructionIntervals();
}

void LivenessAnalysis::computeLocalDefUse() {
    for (const auto& bb : fn_.blocks()) {
        BlockLiveness& liveness = blockMap_[bb.get()];

        for (const auto& inst : bb->instructions()) {
            // Uses
            for (const auto& val : inst->getUses()) {
                auto vreg = std::dynamic_pointer_cast<VirtualRegister>(val);
                if (vreg) {
                    if (liveness.def.find(vreg->id()) == liveness.def.end()) {
                        liveness.use.insert(vreg->id());
                    }
                }
            }

            // Def
            auto def = inst->getDef();
            if (def) {
                liveness.def.insert(def->id());
            }
        }
    }
}

void LivenessAnalysis::computeGlobalDataflow() {
    bool changed = true;
    while (changed) {
        changed = false;

        // Traverse in reverse block order for faster convergence
        for (auto it = fn_.blocks().rbegin(); it != fn_.blocks().rend(); ++it) {
            const BasicBlock* bb = it->get();
            BlockLiveness& liveness = blockMap_[bb];

            // Out[bb] = Union of In[succ] for all succ
            std::unordered_set<int> newOut;
            for (const BasicBlock* succ : bb->successors()) {
                const auto& succIn = blockMap_[succ].liveIn;
                newOut.insert(succIn.begin(), succIn.end());
            }

            // In[bb] = Use[bb] Union (Out[bb] - Def[bb])
            std::unordered_set<int> newIn = liveness.use;
            for (int regId : newOut) {
                if (liveness.def.find(regId) == liveness.def.end()) {
                    newIn.insert(regId);
                }
            }

            if (newIn != liveness.liveIn || newOut != liveness.liveOut) {
                liveness.liveIn = std::move(newIn);
                liveness.liveOut = std::move(newOut);
                changed = true;
            }
        }
    }
}

void LivenessAnalysis::computeInstructionIntervals() {
    liveRanges_.clear();
    std::unordered_map<int, LiveRange> rangeMap;

    int instIndex = 0;
    for (const auto& bb : fn_.blocks()) {
        const auto& liveness = blockMap_[bb.get()];

        // Live-in registers are already live at the start of this block
        for (int regId : liveness.liveIn) {
            auto& lr = rangeMap[regId];
            lr.vregId = regId;
            if (lr.start == -1) lr.start = instIndex;
            lr.end = std::max(lr.end, instIndex);
        }

        for (const auto& inst : bb->instructions()) {
            // Uses update end point
            for (const auto& val : inst->getUses()) {
                auto vreg = std::dynamic_pointer_cast<VirtualRegister>(val);
                if (vreg) {
                    auto& lr = rangeMap[vreg->id()];
                    lr.vregId = vreg->id();
                    lr.isPredicate = vreg->isPredicate();
                    if (lr.start == -1) lr.start = instIndex;
                    lr.end = std::max(lr.end, instIndex);
                }
            }

            // Def sets start point
            auto def = inst->getDef();
            if (def) {
                auto& lr = rangeMap[def->id()];
                lr.vregId = def->id();
                lr.isPredicate = def->isPredicate();
                if (lr.start == -1) lr.start = instIndex;
                lr.end = std::max(lr.end, instIndex);
            }

            instIndex++;
        }

        // Live-out registers remain live until the end of this block
        for (int regId : liveness.liveOut) {
            auto& lr = rangeMap[regId];
            lr.vregId = regId;
            lr.end = std::max(lr.end, instIndex);
        }
    }

    for (auto& pair : rangeMap) {
        liveRanges_.push_back(pair.second);
    }

    // Sort live ranges by start index
    std::sort(liveRanges_.begin(), liveRanges_.end(),
              [](const LiveRange& a, const LiveRange& b) {
                  return a.start < b.start;
              });

    // Compute peak register pressure across all instruction points
    peakPressure_ = 0;
    for (int i = 0; i < instIndex; ++i) {
        int active = 0;
        for (const auto& lr : liveRanges_) {
            if (lr.start <= i && i <= lr.end && !lr.isPredicate) {
                active++;
            }
        }
        peakPressure_ = std::max(peakPressure_, active);
    }
}

void LivenessAnalysis::printReport(std::ostream& out) const {
    out << "============================================================\n";
    out << " Liveness Analysis Report: Function @" << fn_.name() << "\n";
    out << "============================================================\n";

    out << "\n[1] Basic Block Liveness Summary:\n";
    for (const auto& bb : fn_.blocks()) {
        const auto& liveness = blockMap_.at(bb.get());
        out << "  Block %" << bb->name() << ":\n";

        out << "    Def    : {";
        bool first = true;
        for (int r : liveness.def) {
            if (!first) out << ", ";
            out << "%t" << r;
            first = false;
        }
        out << "}\n";

        out << "    Use    : {";
        first = true;
        for (int r : liveness.use) {
            if (!first) out << ", ";
            out << "%t" << r;
            first = false;
        }
        out << "}\n";

        out << "    Live-In: {";
        first = true;
        for (int r : liveness.liveIn) {
            if (!first) out << ", ";
            out << "%t" << r;
            first = false;
        }
        out << "}\n";

        out << "    Live-Out: {";
        first = true;
        for (int r : liveness.liveOut) {
            if (!first) out << ", ";
            out << "%t" << r;
            first = false;
        }
        out << "}\n";
    }

    out << "\n[2] Live Intervals:\n";
    out << "  Register | Type       | Start Inst | End Inst | Span (Length)\n";
    out << "  ---------+------------+------------+----------+--------------\n";
    for (const auto& lr : liveRanges_) {
        std::string regName = (lr.isPredicate ? "%p" : "%t") + std::to_string(lr.vregId);
        std::string typeStr = lr.isPredicate ? "predicate" : "int (s32) ";
        out << "  " << std::left << std::setw(8) << regName
            << " | " << std::left << std::setw(10) << typeStr
            << " | " << std::right << std::setw(10) << lr.start
            << " | " << std::right << std::setw(8) << lr.end
            << " | " << std::right << std::setw(6) << (lr.end - lr.start + 1)
            << "\n";
    }

    out << "\n[3] Peak Register Pressure: " << peakPressure_ << " simultaneous 32-bit registers\n";
    out << "============================================================\n";
}

} // namespace minic::ir
