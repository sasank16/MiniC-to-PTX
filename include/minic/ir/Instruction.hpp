#pragma once

#include "minic/ir/Value.hpp"
#include <string>
#include <memory>
#include <vector>

namespace minic::ir {

class BasicBlock;

enum class Opcode {
    // Arithmetic
    Add,
    Sub,
    Mul,
    SDiv,
    SRem,

    // Unary
    Neg,
    Not,

    // Comparison
    CmpEq,
    CmpNe,
    CmpLt,
    CmpLe,
    CmpGt,
    CmpGe,

    // Memory / Local slots
    Alloca,
    Load,
    Store,

    // Control Flow
    Branch,
    BranchCond,
    Return,

    // Calls
    Call
};

const char* opcodeToString(Opcode op);

class Instruction {
public:
    explicit Instruction(Opcode op) : opcode_(op) {}
    virtual ~Instruction() = default;

    Opcode opcode() const { return opcode_; }
    virtual bool isTerminator() const { return false; }

    virtual std::shared_ptr<VirtualRegister> getDef() const { return nullptr; }
    virtual std::vector<std::shared_ptr<Value>> getUses() const { return {}; }

    virtual std::string toString() const = 0;

private:
    Opcode opcode_;
};

// Binary arithmetic instruction: %dest = op %lhs, %rhs
class BinaryInst : public Instruction {
public:
    BinaryInst(Opcode op, std::shared_ptr<VirtualRegister> dest,
               std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs)
        : Instruction(op), dest_(std::move(dest)), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    std::shared_ptr<VirtualRegister> dest() const { return dest_; }
    std::shared_ptr<Value> lhs() const { return lhs_; }
    std::shared_ptr<Value> rhs() const { return rhs_; }

    void setLhs(std::shared_ptr<Value> v) { lhs_ = std::move(v); }
    void setRhs(std::shared_ptr<Value> v) { rhs_ = std::move(v); }

    std::shared_ptr<VirtualRegister> getDef() const override { return dest_; }
    std::vector<std::shared_ptr<Value>> getUses() const override { return {lhs_, rhs_}; }

    std::string toString() const override;

private:
    std::shared_ptr<VirtualRegister> dest_;
    std::shared_ptr<Value> lhs_;
    std::shared_ptr<Value> rhs_;
};

// Unary instruction: %dest = op %operand
class UnaryInst : public Instruction {
public:
    UnaryInst(Opcode op, std::shared_ptr<VirtualRegister> dest, std::shared_ptr<Value> operand)
        : Instruction(op), dest_(std::move(dest)), operand_(std::move(operand)) {}

    std::shared_ptr<VirtualRegister> dest() const { return dest_; }
    std::shared_ptr<Value> operand() const { return operand_; }
    void setOperand(std::shared_ptr<Value> v) { operand_ = std::move(v); }

    std::shared_ptr<VirtualRegister> getDef() const override { return dest_; }
    std::vector<std::shared_ptr<Value>> getUses() const override { return {operand_}; }

    std::string toString() const override;

private:
    std::shared_ptr<VirtualRegister> dest_;
    std::shared_ptr<Value> operand_;
};

// Comparison instruction producing a 1-bit predicate %p: %p0 = cmp_gt %lhs, %rhs
class CompareInst : public Instruction {
public:
    CompareInst(Opcode op, std::shared_ptr<VirtualRegister> dest,
                std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs)
        : Instruction(op), dest_(std::move(dest)), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    std::shared_ptr<VirtualRegister> dest() const { return dest_; }
    std::shared_ptr<Value> lhs() const { return lhs_; }
    std::shared_ptr<Value> rhs() const { return rhs_; }

    void setLhs(std::shared_ptr<Value> v) { lhs_ = std::move(v); }
    void setRhs(std::shared_ptr<Value> v) { rhs_ = std::move(v); }

    std::shared_ptr<VirtualRegister> getDef() const override { return dest_; }
    std::vector<std::shared_ptr<Value>> getUses() const override { return {lhs_, rhs_}; }

    std::string toString() const override;

private:
    std::shared_ptr<VirtualRegister> dest_;
    std::shared_ptr<Value> lhs_;
    std::shared_ptr<Value> rhs_;
};

// Alloca: %var = alloca int
class AllocaInst : public Instruction {
public:
    AllocaInst(std::string varName, semantic::Type type)
        : Instruction(Opcode::Alloca), varName_(std::move(varName)), type_(type) {}

    const std::string& varName() const { return varName_; }
    semantic::Type allocatedType() const { return type_; }

    std::string toString() const override;

private:
    std::string varName_;
    semantic::Type type_;
};

// Store: store %val, var
class StoreInst : public Instruction {
public:
    StoreInst(std::shared_ptr<Value> val, std::string varName)
        : Instruction(Opcode::Store), val_(std::move(val)), varName_(std::move(varName)) {}

    std::shared_ptr<Value> val() const { return val_; }
    const std::string& varName() const { return varName_; }
    void setVal(std::shared_ptr<Value> v) { val_ = std::move(v); }

    std::vector<std::shared_ptr<Value>> getUses() const override { return {val_}; }

    std::string toString() const override;

private:
    std::shared_ptr<Value> val_;
    std::string varName_;
};

// Load: %dest = load var
class LoadInst : public Instruction {
public:
    LoadInst(std::shared_ptr<VirtualRegister> dest, std::string varName)
        : Instruction(Opcode::Load), dest_(std::move(dest)), varName_(std::move(varName)) {}

    std::shared_ptr<VirtualRegister> dest() const { return dest_; }
    const std::string& varName() const { return varName_; }

    std::shared_ptr<VirtualRegister> getDef() const override { return dest_; }

    std::string toString() const override;

private:
    std::shared_ptr<VirtualRegister> dest_;
    std::string varName_;
};

// Branch (unconditional jump): br label %target
class BranchInst : public Instruction {
public:
    explicit BranchInst(BasicBlock* target)
        : Instruction(Opcode::Branch), target_(target) {}

    BasicBlock* target() const { return target_; }
    void setTarget(BasicBlock* b) { target_ = b; }

    bool isTerminator() const override { return true; }
    std::string toString() const override;

private:
    BasicBlock* target_;
};

// BranchCond (conditional jump): br_cond %p, label %then, label %else
class BranchCondInst : public Instruction {
public:
    BranchCondInst(std::shared_ptr<VirtualRegister> cond, BasicBlock* trueTarget, BasicBlock* falseTarget)
        : Instruction(Opcode::BranchCond), cond_(std::move(cond)),
          trueTarget_(trueTarget), falseTarget_(falseTarget) {}

    std::shared_ptr<VirtualRegister> cond() const { return cond_; }
    BasicBlock* trueTarget() const { return trueTarget_; }
    BasicBlock* falseTarget() const { return falseTarget_; }

    void setTrueTarget(BasicBlock* b) { trueTarget_ = b; }
    void setFalseTarget(BasicBlock* b) { falseTarget_ = b; }

    bool isTerminator() const override { return true; }
    std::vector<std::shared_ptr<Value>> getUses() const override { return {cond_}; }

    std::string toString() const override;

private:
    std::shared_ptr<VirtualRegister> cond_;
    BasicBlock* trueTarget_;
    BasicBlock* falseTarget_;
};

// Return: ret %val or ret void
class ReturnInst : public Instruction {
public:
    explicit ReturnInst(std::shared_ptr<Value> val = nullptr)
        : Instruction(Opcode::Return), val_(std::move(val)) {}

    std::shared_ptr<Value> val() const { return val_; }
    void setVal(std::shared_ptr<Value> v) { val_ = std::move(v); }

    bool isTerminator() const override { return true; }
    std::vector<std::shared_ptr<Value>> getUses() const override {
        if (val_) return {val_};
        return {};
    }

    std::string toString() const override;

private:
    std::shared_ptr<Value> val_;
};

// Call: %dest = call @func(%arg0, %arg1)
class CallInst : public Instruction {
public:
    CallInst(std::shared_ptr<VirtualRegister> dest, std::string callee,
             std::vector<std::shared_ptr<Value>> args)
        : Instruction(Opcode::Call), dest_(std::move(dest)),
          callee_(std::move(callee)), args_(std::move(args)) {}

    std::shared_ptr<VirtualRegister> dest() const { return dest_; }
    const std::string& callee() const { return callee_; }
    const std::vector<std::shared_ptr<Value>>& args() const { return args_; }

    std::shared_ptr<VirtualRegister> getDef() const override { return dest_; }
    std::vector<std::shared_ptr<Value>> getUses() const override { return args_; }

    std::string toString() const override;

private:
    std::shared_ptr<VirtualRegister> dest_;
    std::string callee_;
    std::vector<std::shared_ptr<Value>> args_;
};

} // namespace minic::ir
