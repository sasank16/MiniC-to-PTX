#pragma once

#include "minic/common/SourceLocation.hpp"
#include "minic/ast/ASTVisitor.hpp"
#include <string>
#include <vector>
#include <memory>

namespace minic::ast {

enum class TypeKind {
    Int,
    Void
};

inline const char* typeKindToString(TypeKind type) {
    switch (type) {
        case TypeKind::Int:  return "int";
        case TypeKind::Void: return "void";
    }
    return "unknown";
}

enum class BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Rem,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    And,
    Or
};

inline const char* binaryOpToString(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add: return "+";
        case BinaryOp::Sub: return "-";
        case BinaryOp::Mul: return "*";
        case BinaryOp::Div: return "/";
        case BinaryOp::Rem: return "%";
        case BinaryOp::Eq:  return "==";
        case BinaryOp::Ne:  return "!=";
        case BinaryOp::Lt:  return "<";
        case BinaryOp::Le:  return "<=";
        case BinaryOp::Gt:  return ">";
        case BinaryOp::Ge:  return ">=";
        case BinaryOp::And: return "&&";
        case BinaryOp::Or:  return "||";
    }
    return "?";
}

enum class UnaryOp {
    Plus,
    Neg,
    Not
};

inline const char* unaryOpToString(UnaryOp op) {
    switch (op) {
        case UnaryOp::Plus: return "+";
        case UnaryOp::Neg:  return "-";
        case UnaryOp::Not:  return "!";
    }
    return "?";
}

// Base AST Node
class ASTNode {
public:
    explicit ASTNode(SourceLocation loc) : location_(std::move(loc)) {}
    virtual ~ASTNode() = default;

    const SourceLocation& location() const { return location_; }
    virtual void accept(ASTVisitor& visitor) = 0;

private:
    SourceLocation location_;
};

// Base Expression Node
class Expr : public ASTNode {
public:
    explicit Expr(SourceLocation loc) : ASTNode(std::move(loc)) {}
};

// Base Statement Node
class Stmt : public ASTNode {
public:
    explicit Stmt(SourceLocation loc) : ASTNode(std::move(loc)) {}
};

// Declarations
class ParamDecl : public ASTNode {
public:
    ParamDecl(TypeKind type, std::string name, SourceLocation loc)
        : ASTNode(std::move(loc)), type_(type), name_(std::move(name)) {}

    TypeKind type() const { return type_; }
    const std::string& name() const { return name_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    TypeKind type_;
    std::string name_;
};

class CompoundStmt;

class FunctionDecl : public ASTNode {
public:
    FunctionDecl(TypeKind returnType, std::string name,
                 std::vector<std::unique_ptr<ParamDecl>> params,
                 std::unique_ptr<CompoundStmt> body,
                 bool isKernel,
                 SourceLocation loc)
        : ASTNode(std::move(loc)),
          returnType_(returnType),
          name_(std::move(name)),
          params_(std::move(params)),
          body_(std::move(body)),
          isKernel_(isKernel) {}

    TypeKind returnType() const { return returnType_; }
    const std::string& name() const { return name_; }
    const std::vector<std::unique_ptr<ParamDecl>>& params() const { return params_; }
    CompoundStmt* body() const { return body_.get(); }
    bool isKernel() const { return isKernel_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    TypeKind returnType_;
    std::string name_;
    std::vector<std::unique_ptr<ParamDecl>> params_;
    std::unique_ptr<CompoundStmt> body_;
    bool isKernel_{false};
};

class Program : public ASTNode {
public:
    Program(std::vector<std::unique_ptr<FunctionDecl>> functions, SourceLocation loc)
        : ASTNode(std::move(loc)), functions_(std::move(functions)) {}

    const std::vector<std::unique_ptr<FunctionDecl>>& functions() const { return functions_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::vector<std::unique_ptr<FunctionDecl>> functions_;
};

// Statements
class CompoundStmt : public Stmt {
public:
    CompoundStmt(std::vector<std::unique_ptr<Stmt>> statements, SourceLocation loc)
        : Stmt(std::move(loc)), statements_(std::move(statements)) {}

    const std::vector<std::unique_ptr<Stmt>>& statements() const { return statements_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::vector<std::unique_ptr<Stmt>> statements_;
};

class VarDeclStmt : public Stmt {
public:
    VarDeclStmt(TypeKind type, std::string name, std::unique_ptr<Expr> init, SourceLocation loc)
        : Stmt(std::move(loc)), type_(type), name_(std::move(name)), init_(std::move(init)) {}

    TypeKind type() const { return type_; }
    const std::string& name() const { return name_; }
    Expr* init() const { return init_.get(); }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    TypeKind type_;
    std::string name_;
    std::unique_ptr<Expr> init_;
};

class ExprStmt : public Stmt {
public:
    ExprStmt(std::unique_ptr<Expr> expr, SourceLocation loc)
        : Stmt(std::move(loc)), expr_(std::move(expr)) {}

    Expr* expr() const { return expr_.get(); }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::unique_ptr<Expr> expr_;
};

class IfStmt : public Stmt {
public:
    IfStmt(std::unique_ptr<Expr> condition,
           std::unique_ptr<Stmt> thenBranch,
           std::unique_ptr<Stmt> elseBranch,
           SourceLocation loc)
        : Stmt(std::move(loc)),
          condition_(std::move(condition)),
          thenBranch_(std::move(thenBranch)),
          elseBranch_(std::move(elseBranch)) {}

    Expr* condition() const { return condition_.get(); }
    Stmt* thenBranch() const { return thenBranch_.get(); }
    Stmt* elseBranch() const { return elseBranch_.get(); }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<Stmt> thenBranch_;
    std::unique_ptr<Stmt> elseBranch_;
};

class WhileStmt : public Stmt {
public:
    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body, SourceLocation loc)
        : Stmt(std::move(loc)), condition_(std::move(condition)), body_(std::move(body)) {}

    Expr* condition() const { return condition_.get(); }
    Stmt* body() const { return body_.get(); }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<Stmt> body_;
};

class ReturnStmt : public Stmt {
public:
    ReturnStmt(std::unique_ptr<Expr> value, SourceLocation loc)
        : Stmt(std::move(loc)), value_(std::move(value)) {}

    Expr* value() const { return value_.get(); }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::unique_ptr<Expr> value_;
};

// Expressions
class BinaryExpr : public Expr {
public:
    BinaryExpr(BinaryOp op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, SourceLocation loc)
        : Expr(std::move(loc)), op_(op), left_(std::move(left)), right_(std::move(right)) {}

    BinaryOp op() const { return op_; }
    Expr* left() const { return left_.get(); }
    Expr* right() const { return right_.get(); }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    BinaryOp op_;
    std::unique_ptr<Expr> left_;
    std::unique_ptr<Expr> right_;
};

class UnaryExpr : public Expr {
public:
    UnaryExpr(UnaryOp op, std::unique_ptr<Expr> operand, SourceLocation loc)
        : Expr(std::move(loc)), op_(op), operand_(std::move(operand)) {}

    UnaryOp op() const { return op_; }
    Expr* operand() const { return operand_.get(); }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    UnaryOp op_;
    std::unique_ptr<Expr> operand_;
};

class LiteralExpr : public Expr {
public:
    LiteralExpr(int value, SourceLocation loc)
        : Expr(std::move(loc)), value_(value) {}

    int value() const { return value_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    int value_;
};

class VarExpr : public Expr {
public:
    VarExpr(std::string name, SourceLocation loc)
        : Expr(std::move(loc)), name_(std::move(name)) {}

    const std::string& name() const { return name_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::string name_;
};

class AssignExpr : public Expr {
public:
    AssignExpr(std::string target, std::unique_ptr<Expr> value, SourceLocation loc)
        : Expr(std::move(loc)), target_(std::move(target)), value_(std::move(value)) {}

    const std::string& target() const { return target_; }
    Expr* value() const { return value_.get(); }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::string target_;
    std::unique_ptr<Expr> value_;
};

class CallExpr : public Expr {
public:
    CallExpr(std::string callee, std::vector<std::unique_ptr<Expr>> args, SourceLocation loc)
        : Expr(std::move(loc)), callee_(std::move(callee)), args_(std::move(args)) {}

    const std::string& callee() const { return callee_; }
    const std::vector<std::unique_ptr<Expr>>& args() const { return args_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::string callee_;
    std::vector<std::unique_ptr<Expr>> args_;
};

} // namespace minic::ast
