#pragma once

#include "minic/ast/AST.hpp"
#include "minic/ast/ASTVisitor.hpp"
#include "minic/semantic/SymbolTable.hpp"
#include "minic/common/Diagnostic.hpp"
#include <vector>

namespace minic::semantic {

class SemanticAnalyzer : public ast::ASTVisitor {
public:
    explicit SemanticAnalyzer(DiagnosticEngine& diag) : diag_(diag) {}

    bool analyze(ast::Program& program);

    void visit(ast::Program& node) override;
    void visit(ast::FunctionDecl& node) override;
    void visit(ast::ParamDecl& node) override;

    void visit(ast::CompoundStmt& node) override;
    void visit(ast::VarDeclStmt& node) override;
    void visit(ast::ExprStmt& node) override;
    void visit(ast::IfStmt& node) override;
    void visit(ast::WhileStmt& node) override;
    void visit(ast::ReturnStmt& node) override;

    void visit(ast::BinaryExpr& node) override;
    void visit(ast::UnaryExpr& node) override;
    void visit(ast::LiteralExpr& node) override;
    void visit(ast::VarExpr& node) override;
    void visit(ast::AssignExpr& node) override;
    void visit(ast::CallExpr& node) override;

    Type lastExpressionType() const { return lastExprType_; }

private:
    Type checkExpression(ast::Expr* expr);

    DiagnosticEngine& diag_;
    SymbolTable symTab_;
    const ast::FunctionDecl* currentFunction_{nullptr};
    Type lastExprType_{Type::getError()};
    bool isFunctionBody_{false};
};

} // namespace minic::semantic
