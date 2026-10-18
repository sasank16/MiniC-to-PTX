#pragma once

#include "minic/common/SourceLocation.hpp"
#include <string>
#include <string_view>

namespace minic {

enum class TokenType {
    // End of file / Error
    EndOfFile,
    Invalid,

    // Identifiers and Literals
    Identifier,
    IntegerLiteral,

    // Keywords
    KwInt,
    KwVoid,
    KwIf,
    KwElse,
    KwWhile,
    KwReturn,
    KwGlobal,
    KwDevice,

    // Arithmetic Operators
    Plus,          // +
    Minus,         // -
    Star,          // *
    Slash,         // /
    Percent,       // %

    // Comparison Operators
    EqualEqual,    // ==
    ExclaimEqual,  // !=
    Less,          // <
    LessEqual,     // <=
    Greater,       // >
    GreaterEqual,  // >=

    // Logical Operators
    AmpAmp,        // &&
    PipePipe,      // ||
    Exclaim,       // !

    // Assignment Operator
    Equal,         // =

    // Delimiters and Punctuation
    Semicolon,     // ;
    Comma,         // ,
    LParen,        // (
    RParen,        // )
    LBrace,        // {
    RBrace         // }
};

const char* tokenTypeToString(TokenType type);

struct Token {
    TokenType type{TokenType::Invalid};
    std::string lexeme;
    int intValue{0}; // Valid when type == IntegerLiteral
    SourceLocation location;

    Token() = default;
    Token(TokenType t, std::string lex, SourceLocation loc)
        : type(t), lexeme(std::move(lex)), location(std::move(loc)) {}
    Token(TokenType t, std::string lex, int val, SourceLocation loc)
        : type(t), lexeme(std::move(lex)), intValue(val), location(std::move(loc)) {}

    bool is(TokenType t) const { return type == t; }
    bool isNot(TokenType t) const { return type != t; }
};

} // namespace minic
