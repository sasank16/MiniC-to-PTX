#pragma once

#include "minic/ast/AST.hpp"
#include <iostream>
#include <string>

namespace minic::ast {

class ASTDumper : public ASTVisitor {
public:
    explicit ASTDumper(std::ostream& out = std::cout) : out_(out) {}

    void dump(ASTNode& node);

    void visit(Program& node) override;
    void visit(FunctionDecl& node) override;
    void visit(ParamDecl& node) override;

    void visit(CompoundStmt& node) override;
    void visit(VarDeclStmt& node) override;
    void visit(ExprStmt& node) override;
    void visit(IfStmt& node) override;
    void visit(WhileStmt& node) override;
    void visit(ReturnStmt& node) override;

    void visit(BinaryExpr& node) override;
    void visit(UnaryExpr& node) override;
    void visit(LiteralExpr& node) override;
    void visit(VarExpr& node) override;
    void visit(AssignExpr& node) override;
    void visit(CallExpr& node) override;

private:
    void indent();
    std::ostream& out_;
    int indentLevel_{0};
};

} // namespace minic::ast
