#pragma once

#include "minic/lexer/Lexer.hpp"
#include "minic/ast/AST.hpp"
#include "minic/common/Diagnostic.hpp"
#include <memory>
#include <vector>

namespace minic::parser {

class Parser {
public:
    Parser(Lexer& lexer, DiagnosticEngine& diag);

    std::unique_ptr<ast::Program> parseProgram();

private:
    const Token& peek() const;
    const Token& previous() const;
    bool isAtEnd() const;
    bool check(TokenType type) const;
    Token advance();
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& errorMessage);
    void synchronize();

    // Declarations
    std::unique_ptr<ast::FunctionDecl> parseFunction();
    std::unique_ptr<ast::ParamDecl> parseParam();
    ast::TypeKind parseType();

    // Statements
    std::unique_ptr<ast::Stmt> parseStatement();
    std::unique_ptr<ast::CompoundStmt> parseCompoundStmt();
    std::unique_ptr<ast::VarDeclStmt> parseVarDeclStmt();
    std::unique_ptr<ast::IfStmt> parseIfStmt();
    std::unique_ptr<ast::WhileStmt> parseWhileStmt();
    std::unique_ptr<ast::ReturnStmt> parseReturnStmt();
    std::unique_ptr<ast::ExprStmt> parseExprStmt();

    // Expressions (Pratt / Precedence Climbing)
    std::unique_ptr<ast::Expr> parseExpression();
    std::unique_ptr<ast::Expr> parseAssignment();
    std::unique_ptr<ast::Expr> parseLogicalOr();
    std::unique_ptr<ast::Expr> parseLogicalAnd();
    std::unique_ptr<ast::Expr> parseEquality();
    std::unique_ptr<ast::Expr> parseRelational();
    std::unique_ptr<ast::Expr> parseAdditive();
    std::unique_ptr<ast::Expr> parseMultiplicative();
    std::unique_ptr<ast::Expr> parseUnary();
    std::unique_ptr<ast::Expr> parsePrimary();

    Lexer& lexer_;
    DiagnosticEngine& diag_;
    std::vector<Token> tokens_;
    size_t current_{0};
};

} // namespace minic::parser
