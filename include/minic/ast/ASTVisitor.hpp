#pragma once

namespace minic::ast {

// Forward declarations of AST nodes
class Program;
class FunctionDecl;
class ParamDecl;
class CompoundStmt;
class VarDeclStmt;
class ExprStmt;
class IfStmt;
class WhileStmt;
class ReturnStmt;
class BinaryExpr;
class UnaryExpr;
class LiteralExpr;
class VarExpr;
class AssignExpr;
class CallExpr;

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(Program& node) = 0;
    virtual void visit(FunctionDecl& node) = 0;
    virtual void visit(ParamDecl& node) = 0;

    virtual void visit(CompoundStmt& node) = 0;
    virtual void visit(VarDeclStmt& node) = 0;
    virtual void visit(ExprStmt& node) = 0;
    virtual void visit(IfStmt& node) = 0;
    virtual void visit(WhileStmt& node) = 0;
    virtual void visit(ReturnStmt& node) = 0;

    virtual void visit(BinaryExpr& node) = 0;
    virtual void visit(UnaryExpr& node) = 0;
    virtual void visit(LiteralExpr& node) = 0;
    virtual void visit(VarExpr& node) = 0;
    virtual void visit(AssignExpr& node) = 0;
    virtual void visit(CallExpr& node) = 0;
};

} // namespace minic::ast
