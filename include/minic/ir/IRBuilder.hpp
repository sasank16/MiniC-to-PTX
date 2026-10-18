#pragma once

#include "minic/ast/AST.hpp"
#include "minic/ast/ASTVisitor.hpp"
#include "minic/ir/Module.hpp"
#include "minic/common/Diagnostic.hpp"
#include <memory>
#include <string>

namespace minic::ir {

class IRBuilder : public ast::ASTVisitor {
public:
    explicit IRBuilder(std::string moduleName)
        : module_(std::make_unique<Module>(std::move(moduleName))) {}

    std::unique_ptr<Module> build(ast::Program& program);

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

private:
    std::shared_ptr<Value> lowerExpression(ast::Expr* expr);
    std::shared_ptr<VirtualRegister> ensurePredicate(std::shared_ptr<Value> val);
    void connectEdge(BasicBlock* from, BasicBlock* to);

    std::unique_ptr<Module> module_;
    Function* currentFunction_{nullptr};
    BasicBlock* currentBlock_{nullptr};
    std::shared_ptr<Value> lastValue_{nullptr};
};

} // namespace minic::ir
