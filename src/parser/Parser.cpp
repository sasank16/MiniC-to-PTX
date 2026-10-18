#include "minic/parser/Parser.hpp"

namespace minic::parser {

Parser::Parser(Lexer& lexer, DiagnosticEngine& diag)
    : lexer_(lexer), diag_(diag) {
    tokens_ = lexer_.tokenizeAll();
}

const Token& Parser::peek() const {
    return tokens_[current_];
}

const Token& Parser::previous() const {
    return tokens_[current_ - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EndOfFile;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

Token Parser::advance() {
    if (!isAtEnd()) {
        current_++;
    }
    return previous();
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& errorMessage) {
    if (check(type)) {
        return advance();
    }
    diag_.error(peek().location, errorMessage + " (found '" + peek().lexeme + "')");
    return peek();
}

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (previous().type == TokenType::Semicolon) return;

        switch (peek().type) {
            case TokenType::KwInt:
            case TokenType::KwVoid:
            case TokenType::KwIf:
            case TokenType::KwWhile:
            case TokenType::KwReturn:
            case TokenType::LBrace:
            case TokenType::RBrace:
                return;
            default:
                break;
        }
        advance();
    }
}

ast::TypeKind Parser::parseType() {
    if (match(TokenType::KwInt)) {
        return ast::TypeKind::Int;
    }
    if (match(TokenType::KwVoid)) {
        return ast::TypeKind::Void;
    }
    diag_.error(peek().location, "Expected type specifier ('int' or 'void')");
    return ast::TypeKind::Int;
}

std::unique_ptr<ast::ParamDecl> Parser::parseParam() {
    SourceLocation loc = peek().location;
    ast::TypeKind type = parseType();
    Token nameTok = consume(TokenType::Identifier, "Expected parameter name");
    return std::make_unique<ast::ParamDecl>(type, nameTok.lexeme, loc);
}

std::unique_ptr<ast::FunctionDecl> Parser::parseFunction() {
    SourceLocation loc = peek().location;
    bool isKernel = false;

    if (match(TokenType::KwGlobal)) {
        isKernel = true;
    } else if (match(TokenType::KwDevice)) {
        isKernel = false;
    }

    ast::TypeKind returnType = parseType();
    Token nameTok = consume(TokenType::Identifier, "Expected function name");

    consume(TokenType::LParen, "Expected '(' after function name");
    std::vector<std::unique_ptr<ast::ParamDecl>> params;
    if (!check(TokenType::RParen)) {
        do {
            params.push_back(parseParam());
        } while (match(TokenType::Comma));
    }
    consume(TokenType::RParen, "Expected ')' after parameters");

    std::unique_ptr<ast::CompoundStmt> body = parseCompoundStmt();
    return std::make_unique<ast::FunctionDecl>(
        returnType, nameTok.lexeme, std::move(params), std::move(body), isKernel, loc);
}

std::unique_ptr<ast::CompoundStmt> Parser::parseCompoundStmt() {
    SourceLocation loc = peek().location;
    consume(TokenType::LBrace, "Expected '{'");

    std::vector<std::unique_ptr<ast::Stmt>> stmts;
    while (!check(TokenType::RBrace) && !isAtEnd()) {
        try {
            auto s = parseStatement();
            if (s) {
                stmts.push_back(std::move(s));
            }
        } catch (...) {
            synchronize();
        }
    }
    consume(TokenType::RBrace, "Expected '}'");
    return std::make_unique<ast::CompoundStmt>(std::move(stmts), loc);
}

std::unique_ptr<ast::VarDeclStmt> Parser::parseVarDeclStmt() {
    SourceLocation loc = peek().location;
    ast::TypeKind type = parseType();
    Token nameTok = consume(TokenType::Identifier, "Expected variable name");

    std::unique_ptr<ast::Expr> init = nullptr;
    if (match(TokenType::Equal)) {
        init = parseExpression();
    }
    consume(TokenType::Semicolon, "Expected ';' after variable declaration");
    return std::make_unique<ast::VarDeclStmt>(type, nameTok.lexeme, std::move(init), loc);
}

std::unique_ptr<ast::IfStmt> Parser::parseIfStmt() {
    SourceLocation loc = peek().location;
    consume(TokenType::KwIf, "Expected 'if'");
    consume(TokenType::LParen, "Expected '(' after 'if'");
    auto cond = parseExpression();
    consume(TokenType::RParen, "Expected ')' after if condition");

    auto thenBranch = parseStatement();
    std::unique_ptr<ast::Stmt> elseBranch = nullptr;
    if (match(TokenType::KwElse)) {
        elseBranch = parseStatement();
    }
    return std::make_unique<ast::IfStmt>(
        std::move(cond), std::move(thenBranch), std::move(elseBranch), loc);
}

std::unique_ptr<ast::WhileStmt> Parser::parseWhileStmt() {
    SourceLocation loc = peek().location;
    consume(TokenType::KwWhile, "Expected 'while'");
    consume(TokenType::LParen, "Expected '(' after 'while'");
    auto cond = parseExpression();
    consume(TokenType::RParen, "Expected ')' after while condition");

    auto body = parseStatement();
    return std::make_unique<ast::WhileStmt>(std::move(cond), std::move(body), loc);
}

std::unique_ptr<ast::ReturnStmt> Parser::parseReturnStmt() {
    SourceLocation loc = peek().location;
    consume(TokenType::KwReturn, "Expected 'return'");

    std::unique_ptr<ast::Expr> val = nullptr;
    if (!check(TokenType::Semicolon)) {
        val = parseExpression();
    }
    consume(TokenType::Semicolon, "Expected ';' after return value");
    return std::make_unique<ast::ReturnStmt>(std::move(val), loc);
}

std::unique_ptr<ast::ExprStmt> Parser::parseExprStmt() {
    SourceLocation loc = peek().location;
    std::unique_ptr<ast::Expr> expr = nullptr;
    if (!check(TokenType::Semicolon)) {
        expr = parseExpression();
    }
    consume(TokenType::Semicolon, "Expected ';' after expression");
    return std::make_unique<ast::ExprStmt>(std::move(expr), loc);
}

std::unique_ptr<ast::Stmt> Parser::parseStatement() {
    if (check(TokenType::KwInt) || check(TokenType::KwVoid)) {
        return parseVarDeclStmt();
    }
    if (check(TokenType::KwIf)) {
        return parseIfStmt();
    }
    if (check(TokenType::KwWhile)) {
        return parseWhileStmt();
    }
    if (check(TokenType::KwReturn)) {
        return parseReturnStmt();
    }
    if (check(TokenType::LBrace)) {
        return parseCompoundStmt();
    }
    return parseExprStmt();
}

std::unique_ptr<ast::Program> Parser::parseProgram() {
    SourceLocation loc = peek().location;
    std::vector<std::unique_ptr<ast::FunctionDecl>> functions;

    while (!isAtEnd()) {
        try {
            functions.push_back(parseFunction());
        } catch (...) {
            synchronize();
        }
    }

    return std::make_unique<ast::Program>(std::move(functions), loc);
}

// Expression parsing with operator precedence climbing
std::unique_ptr<ast::Expr> Parser::parseExpression() {
    return parseAssignment();
}

std::unique_ptr<ast::Expr> Parser::parseAssignment() {
    auto expr = parseLogicalOr();

    if (match(TokenType::Equal)) {
        SourceLocation loc = previous().location;
        auto value = parseAssignment(); // Right-associative

        auto* varExpr = dynamic_cast<ast::VarExpr*>(expr.get());
        if (varExpr) {
            std::string name = varExpr->name();
            return std::make_unique<ast::AssignExpr>(std::move(name), std::move(value), loc);
        }
        diag_.error(loc, "Invalid target for assignment (lvalue required)");
    }
    return expr;
}

std::unique_ptr<ast::Expr> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    while (match(TokenType::PipePipe)) {
        SourceLocation loc = previous().location;
        auto right = parseLogicalAnd();
        left = std::make_unique<ast::BinaryExpr>(
            ast::BinaryOp::Or, std::move(left), std::move(right), loc);
    }
    return left;
}

std::unique_ptr<ast::Expr> Parser::parseLogicalAnd() {
    auto left = parseEquality();
    while (match(TokenType::AmpAmp)) {
        SourceLocation loc = previous().location;
        auto right = parseEquality();
        left = std::make_unique<ast::BinaryExpr>(
            ast::BinaryOp::And, std::move(left), std::move(right), loc);
    }
    return left;
}

std::unique_ptr<ast::Expr> Parser::parseEquality() {
    auto left = parseRelational();
    while (check(TokenType::EqualEqual) || check(TokenType::ExclaimEqual)) {
        Token opTok = advance();
        ast::BinaryOp op = (opTok.type == TokenType::EqualEqual)
                               ? ast::BinaryOp::Eq
                               : ast::BinaryOp::Ne;
        auto right = parseRelational();
        left = std::make_unique<ast::BinaryExpr>(op, std::move(left), std::move(right), opTok.location);
    }
    return left;
}

std::unique_ptr<ast::Expr> Parser::parseRelational() {
    auto left = parseAdditive();
    while (check(TokenType::Less) || check(TokenType::LessEqual) ||
           check(TokenType::Greater) || check(TokenType::GreaterEqual)) {
        Token opTok = advance();
        ast::BinaryOp op;
        switch (opTok.type) {
            case TokenType::Less:         op = ast::BinaryOp::Lt; break;
            case TokenType::LessEqual:    op = ast::BinaryOp::Le; break;
            case TokenType::Greater:      op = ast::BinaryOp::Gt; break;
            case TokenType::GreaterEqual: op = ast::BinaryOp::Ge; break;
            default: op = ast::BinaryOp::Lt; break;
        }
        auto right = parseAdditive();
        left = std::make_unique<ast::BinaryExpr>(op, std::move(left), std::move(right), opTok.location);
    }
    return left;
}

std::unique_ptr<ast::Expr> Parser::parseAdditive() {
    auto left = parseMultiplicative();
    while (check(TokenType::Plus) || check(TokenType::Minus)) {
        Token opTok = advance();
        ast::BinaryOp op = (opTok.type == TokenType::Plus) ? ast::BinaryOp::Add : ast::BinaryOp::Sub;
        auto right = parseMultiplicative();
        left = std::make_unique<ast::BinaryExpr>(op, std::move(left), std::move(right), opTok.location);
    }
    return left;
}

std::unique_ptr<ast::Expr> Parser::parseMultiplicative() {
    auto left = parseUnary();
    while (check(TokenType::Star) || check(TokenType::Slash) || check(TokenType::Percent)) {
        Token opTok = advance();
        ast::BinaryOp op;
        switch (opTok.type) {
            case TokenType::Star:    op = ast::BinaryOp::Mul; break;
            case TokenType::Slash:   op = ast::BinaryOp::Div; break;
            case TokenType::Percent: op = ast::BinaryOp::Rem; break;
            default: op = ast::BinaryOp::Mul; break;
        }
        auto right = parseUnary();
        left = std::make_unique<ast::BinaryExpr>(op, std::move(left), std::move(right), opTok.location);
    }
    return left;
}

std::unique_ptr<ast::Expr> Parser::parseUnary() {
    if (check(TokenType::Plus) || check(TokenType::Minus) || check(TokenType::Exclaim)) {
        Token opTok = advance();
        ast::UnaryOp op;
        switch (opTok.type) {
            case TokenType::Plus:    op = ast::UnaryOp::Plus; break;
            case TokenType::Minus:   op = ast::UnaryOp::Neg; break;
            case TokenType::Exclaim: op = ast::UnaryOp::Not; break;
            default: op = ast::UnaryOp::Plus; break;
        }
        auto operand = parseUnary();
        return std::make_unique<ast::UnaryExpr>(op, std::move(operand), opTok.location);
    }
    return parsePrimary();
}

std::unique_ptr<ast::Expr> Parser::parsePrimary() {
    if (match(TokenType::IntegerLiteral)) {
        return std::make_unique<ast::LiteralExpr>(previous().intValue, previous().location);
    }

    if (match(TokenType::Identifier)) {
        Token idTok = previous();
        // Check for function call
        if (match(TokenType::LParen)) {
            std::vector<std::unique_ptr<ast::Expr>> args;
            if (!check(TokenType::RParen)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RParen, "Expected ')' after call arguments");
            return std::make_unique<ast::CallExpr>(idTok.lexeme, std::move(args), idTok.location);
        }
        return std::make_unique<ast::VarExpr>(idTok.lexeme, idTok.location);
    }

    if (match(TokenType::LParen)) {
        auto expr = parseExpression();
        consume(TokenType::RParen, "Expected ')' after expression");
        return expr;
    }

    diag_.error(peek().location, "Expected expression (found '" + peek().lexeme + "')");
    advance(); // Skip unexpected token to avoid infinite loop
    return std::make_unique<ast::LiteralExpr>(0, peek().location);
}

} // namespace minic::parser
