#pragma once

#include "minic/lexer/Token.hpp"
#include "minic/common/Diagnostic.hpp"
#include <string>
#include <vector>

namespace minic {

class Lexer {
public:
    Lexer(std::string source, std::string filename, DiagnosticEngine& diag);

    Token nextToken();
    std::vector<Token> tokenizeAll();

private:
    char peek() const;
    char peekNext() const;
    char advance();
    bool isAtEnd() const;
    bool match(char expected);

    void skipWhitespaceAndComments();
    Token makeToken(TokenType type);
    Token makeToken(TokenType type, std::string lexeme);
    Token lexNumber();
    Token lexIdentifierOrKeyword();

    std::string source_;
    std::string filename_;
    DiagnosticEngine& diag_;

    size_t start_{0};
    size_t current_{0};
    int line_{1};
    int column_{1};
    int tokenStartCol_{1};
};

} // namespace minic
