#include "minic/ir/Instruction.hpp"
#include "minic/ir/BasicBlock.hpp"
#include "minic/ir/Function.hpp"
#include <sstream>

namespace minic::ir {

const char* opcodeToString(Opcode op) {
    switch (op) {
        case Opcode::Add:        return "add";
        case Opcode::Sub:        return "sub";
        case Opcode::Mul:        return "mul";
        case Opcode::SDiv:       return "sdiv";
        case Opcode::SRem:       return "srem";
        case Opcode::Neg:        return "neg";
        case Opcode::Not:        return "not";
        case Opcode::CmpEq:      return "cmp_eq";
        case Opcode::CmpNe:      return "cmp_ne";
        case Opcode::CmpLt:      return "cmp_lt";
        case Opcode::CmpLe:      return "cmp_le";
        case Opcode::CmpGt:      return "cmp_gt";
        case Opcode::CmpGe:      return "cmp_ge";
        case Opcode::Alloca:     return "alloca";
        case Opcode::Load:       return "load";
        case Opcode::Store:      return "store";
        case Opcode::Branch:     return "br";
        case Opcode::BranchCond: return "br_cond";
        case Opcode::Return:     return "ret";
        case Opcode::Call:       return "call";
    }
    return "unknown";
}

std::string BinaryInst::toString() const {
    return dest_->toString() + " = " + opcodeToString(opcode()) + " " +
           lhs_->toString() + ", " + rhs_->toString();
}

std::string UnaryInst::toString() const {
    return dest_->toString() + " = " + opcodeToString(opcode()) + " " + operand_->toString();
}

std::string CompareInst::toString() const {
    return dest_->toString() + " = " + opcodeToString(opcode()) + " " +
           lhs_->toString() + ", " + rhs_->toString();
}

std::string AllocaInst::toString() const {
    return "%" + varName_ + " = alloca " + type_.toString();
}

std::string StoreInst::toString() const {
    return "store " + val_->toString() + ", %" + varName_;
}

std::string LoadInst::toString() const {
    return dest_->toString() + " = load %" + varName_;
}

std::string BranchInst::toString() const {
    return "br label %" + target_->name();
}

std::string BranchCondInst::toString() const {
    return "br_cond " + cond_->toString() + ", label %" +
           trueTarget_->name() + ", label %" + falseTarget_->name();
}

std::string ReturnInst::toString() const {
    if (val_) {
        return "ret " + val_->toString();
    }
    return "ret void";
}

std::string CallInst::toString() const {
    std::ostringstream oss;
    if (dest_) {
        oss << dest_->toString() << " = ";
    }
    oss << "call @" << callee_ << "(";
    for (size_t i = 0; i < args_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << args_[i]->toString();
    }
    oss << ")";
    return oss.str();
}

void BasicBlock::print(std::ostream& out) const {
    out << name_ << ":";
    if (!predecessors_.empty()) {
        out << " ; preds =";
        for (size_t i = 0; i < predecessors_.size(); ++i) {
            out << " %" << predecessors_[i]->name();
        }
    }
    out << "\n";

    for (const auto& inst : instructions_) {
        out << "    " << inst->toString() << "\n";
    }
}

void Function::print(std::ostream& out) const {
    out << "define " << (isKernel_ ? "kernel " : "") << returnType_.toString()
        << " @" << name_ << "(";
    for (size_t i = 0; i < params_.size(); ++i) {
        if (i > 0) out << ", ";
        out << params_[i]->type().toString() << " " << params_[i]->toString();
    }
    out << ") {\n";

    for (const auto& bb : blocks_) {
        bb->print(out);
    }
    out << "}\n";
}

void Function::printCFGDot(std::ostream& out) const {
    out << "digraph \"" << name_ << "\" {\n";
    out << "    node [shape=record, fontname=\"Courier\"];\n";

    for (const auto& bb : blocks_) {
        std::ostringstream label;
        label << "{" << bb->name() << ":|";
        for (const auto& inst : bb->instructions()) {
            std::string s = inst->toString();
            // Escape special chars for graphviz record labels
            for (char c : s) {
                if (c == '{' || c == '}' || c == '<' || c == '>' || c == '|' || c == '"') {
                    label << '\\';
                }
                label << c;
            }
            label << "\\l";
        }
        label << "}";

        out << "    \"" << bb->name() << "\" [label=\"" << label.str() << "\"];\n";

        for (auto* succ : bb->successors()) {
            out << "    \"" << bb->name() << "\" -> \"" << succ->name() << "\";\n";
        }
    }
    out << "}\n";
}

} // namespace minic::ir
