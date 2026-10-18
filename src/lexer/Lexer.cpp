#include "minic/lexer/Lexer.hpp"
#include <cctype>
#include <unordered_map>

namespace minic {

const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::EndOfFile:      return "EndOfFile";
        case TokenType::Invalid:        return "Invalid";
        case TokenType::Identifier:     return "Identifier";
        case TokenType::IntegerLiteral: return "IntegerLiteral";
        case TokenType::KwInt:          return "int";
        case TokenType::KwVoid:         return "void";
        case TokenType::KwIf:           return "if";
        case TokenType::KwElse:         return "else";
        case TokenType::KwWhile:        return "while";
        case TokenType::KwReturn:       return "return";
        case TokenType::KwGlobal:       return "__global__";
        case TokenType::KwDevice:       return "__device__";
        case TokenType::Plus:           return "+";
        case TokenType::Minus:          return "-";
        case TokenType::Star:           return "*";
        case TokenType::Slash:          return "/";
        case TokenType::Percent:        return "%";
        case TokenType::EqualEqual:     return "==";
        case TokenType::ExclaimEqual:   return "!=";
        case TokenType::Less:           return "<";
        case TokenType::LessEqual:      return "<=";
        case TokenType::Greater:        return ">";
        case TokenType::GreaterEqual:   return ">=";
        case TokenType::AmpAmp:         return "&&";
        case TokenType::PipePipe:       return "||";
        case TokenType::Exclaim:        return "!";
        case TokenType::Equal:          return "=";
        case TokenType::Semicolon:      return ";";
        case TokenType::Comma:          return ",";
        case TokenType::LParen:         return "(";
        case TokenType::RParen:         return ")";
        case TokenType::LBrace:         return "{";
        case TokenType::RBrace:         return "}";
    }
    return "Unknown";
}

static const std::unordered_map<std::string, TokenType> kKeywords = {
    {"int",        TokenType::KwInt},
    {"void",       TokenType::KwVoid},
    {"if",         TokenType::KwIf},
    {"else",       TokenType::KwElse},
    {"while",      TokenType::KwWhile},
    {"return",     TokenType::KwReturn},
    {"__global__", TokenType::KwGlobal},
    {"__device__", TokenType::KwDevice}
};

Lexer::Lexer(std::string source, std::string filename, DiagnosticEngine& diag)
    : source_(std::move(source)), filename_(std::move(filename)), diag_(diag) {}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= source_.size()) return '\0';
    return source_[current_ + 1];
}

char Lexer::advance() {
    char c = source_[current_++];
    column_++;
    return c;
}

bool Lexer::isAtEnd() const {
    return current_ >= source_.size();
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source_[current_] != expected) {
        return false;
    }
    current_++;
    column_++;
    return true;
}

Token Lexer::makeToken(TokenType type) {
    std::string lexeme = source_.substr(start_, current_ - start_);
    return Token(type, lexeme, SourceLocation(filename_, line_, tokenStartCol_));
}

Token Lexer::makeToken(TokenType type, std::string lexeme) {
    return Token(type, std::move(lexeme), SourceLocation(filename_, line_, tokenStartCol_));
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\n') {
            line_++;
            column_ = 0;
            advance();
        } else if (c == '/' && peekNext() == '/') {
            // Line comment: skip until newline or end of file
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
        } else if (c == '/' && peekNext() == '*') {
            // Block comment: skip until */
            int commentStartLine = line_;
            int commentStartCol = column_;
            advance(); // Consume '/'
            advance(); // Consume '*'
            bool closed = false;
            while (!isAtEnd()) {
                if (peek() == '*' && peekNext() == '/') {
                    advance(); // '*'
                    advance(); // '/'
                    closed = true;
                    break;
                }
                if (peek() == '\n') {
                    line_++;
                    column_ = 0;
                }
                advance();
            }
            if (!closed) {
                diag_.error(SourceLocation(filename_, commentStartLine, commentStartCol),
                            "Unterminated block comment");
            }
        } else {
            break;
        }
    }
}

Token Lexer::lexNumber() {
    while (std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }
    std::string text = source_.substr(start_, current_ - start_);
    int val = 0;
    try {
        val = std::stoi(text);
    } catch (...) {
        diag_.error(SourceLocation(filename_, line_, tokenStartCol_),
                    "Integer literal '" + text + "' out of range for 32-bit signed integer");
    }
    return Token(TokenType::IntegerLiteral, text, val,
                 SourceLocation(filename_, line_, tokenStartCol_));
}

Token Lexer::lexIdentifierOrKeyword() {
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
        advance();
    }
    std::string text = source_.substr(start_, current_ - start_);
    auto it = kKeywords.find(text);
    TokenType type = (it != kKeywords.end()) ? it->second : TokenType::Identifier;
    return makeToken(type, text);
}

Token Lexer::nextToken() {
    skipWhitespaceAndComments();

    start_ = current_;
    tokenStartCol_ = column_;

    if (isAtEnd()) {
        return Token(TokenType::EndOfFile, "", SourceLocation(filename_, line_, column_));
    }

    char c = advance();

    if (std::isdigit(static_cast<unsigned char>(c))) {
        return lexNumber();
    }

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return lexIdentifierOrKeyword();
    }

    switch (c) {
        case '(': return makeToken(TokenType::LParen);
        case ')': return makeToken(TokenType::RParen);
        case '{': return makeToken(TokenType::LBrace);
        case '}': return makeToken(TokenType::RBrace);
        case ';': return makeToken(TokenType::Semicolon);
        case ',': return makeToken(TokenType::Comma);
        case '+': return makeToken(TokenType::Plus);
        case '-': return makeToken(TokenType::Minus);
        case '*': return makeToken(TokenType::Star);
        case '/': return makeToken(TokenType::Slash);
        case '%': return makeToken(TokenType::Percent);
        case '=':
            return match('=') ? makeToken(TokenType::EqualEqual) : makeToken(TokenType::Equal);
        case '!':
            return match('=') ? makeToken(TokenType::ExclaimEqual) : makeToken(TokenType::Exclaim);
        case '<':
            return match('=') ? makeToken(TokenType::LessEqual) : makeToken(TokenType::Less);
        case '>':
            return match('=') ? makeToken(TokenType::GreaterEqual) : makeToken(TokenType::Greater);
        case '&':
            if (match('&')) {
                return makeToken(TokenType::AmpAmp);
            }
            diag_.error(SourceLocation(filename_, line_, tokenStartCol_),
                        "Unexpected character '&' (did you mean '&&'?)");
            return makeToken(TokenType::Invalid);
        case '|':
            if (match('|')) {
                return makeToken(TokenType::PipePipe);
            }
            diag_.error(SourceLocation(filename_, line_, tokenStartCol_),
                        "Unexpected character '|' (did you mean '||'?)");
            return makeToken(TokenType::Invalid);
        default:
            diag_.error(SourceLocation(filename_, line_, tokenStartCol_),
                        std::string("Unexpected character '") + c + "'");
            return makeToken(TokenType::Invalid);
    }
}

std::vector<Token> Lexer::tokenizeAll() {
    std::vector<Token> tokens;
    while (true) {
        Token tok = nextToken();
        tokens.push_back(tok);
        if (tok.is(TokenType::EndOfFile)) {
            break;
        }
    }
    return tokens;
}

} // namespace minic
