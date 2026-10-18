#include "tests/test_framework.hpp"
#include "minic/lexer/Lexer.hpp"
#include "minic/common/Diagnostic.hpp"

using namespace minic;

TEST(LexerTest, KeywordsAndIdentifiers) {
    DiagnosticEngine diag;
    std::string src = "int void if else while return __global__ __device__ my_var x123";
    Lexer lexer(src, "test.mc", diag);
    auto tokens = lexer.tokenizeAll();

    EXPECT_FALSE(diag.hasErrors());
    EXPECT_EQ(tokens.size(), 11u); // 10 tokens + EOF

    EXPECT_TRUE(tokens[0].is(TokenType::KwInt));
    EXPECT_TRUE(tokens[1].is(TokenType::KwVoid));
    EXPECT_TRUE(tokens[2].is(TokenType::KwIf));
    EXPECT_TRUE(tokens[3].is(TokenType::KwElse));
    EXPECT_TRUE(tokens[4].is(TokenType::KwWhile));
    EXPECT_TRUE(tokens[5].is(TokenType::KwReturn));
    EXPECT_TRUE(tokens[6].is(TokenType::KwGlobal));
    EXPECT_TRUE(tokens[7].is(TokenType::KwDevice));
    EXPECT_TRUE(tokens[8].is(TokenType::Identifier));
    EXPECT_EQ(tokens[8].lexeme, "my_var");
    EXPECT_TRUE(tokens[9].is(TokenType::Identifier));
    EXPECT_EQ(tokens[9].lexeme, "x123");
    EXPECT_TRUE(tokens[10].is(TokenType::EndOfFile));
}

TEST(LexerTest, NumbersAndValues) {
    DiagnosticEngine diag;
    std::string src = "0 42 1000 2147483647";
    Lexer lexer(src, "test.mc", diag);
    auto tokens = lexer.tokenizeAll();

    EXPECT_FALSE(diag.hasErrors());
    EXPECT_EQ(tokens.size(), 5u);

    EXPECT_EQ(tokens[0].intValue, 0);
    EXPECT_EQ(tokens[1].intValue, 42);
    EXPECT_EQ(tokens[2].intValue, 1000);
    EXPECT_EQ(tokens[3].intValue, 2147483647);
}

TEST(LexerTest, OperatorsAndPunctuation) {
    DiagnosticEngine diag;
    std::string src = "+ - * / % == != < <= > >= && || ! = ; , ( ) { }";
    Lexer lexer(src, "test.mc", diag);
    auto tokens = lexer.tokenizeAll();

    EXPECT_FALSE(diag.hasErrors());
    EXPECT_TRUE(tokens[0].is(TokenType::Plus));
    EXPECT_TRUE(tokens[1].is(TokenType::Minus));
    EXPECT_TRUE(tokens[2].is(TokenType::Star));
    EXPECT_TRUE(tokens[3].is(TokenType::Slash));
    EXPECT_TRUE(tokens[4].is(TokenType::Percent));
    EXPECT_TRUE(tokens[5].is(TokenType::EqualEqual));
    EXPECT_TRUE(tokens[6].is(TokenType::ExclaimEqual));
    EXPECT_TRUE(tokens[7].is(TokenType::Less));
    EXPECT_TRUE(tokens[8].is(TokenType::LessEqual));
    EXPECT_TRUE(tokens[9].is(TokenType::Greater));
    EXPECT_TRUE(tokens[10].is(TokenType::GreaterEqual));
    EXPECT_TRUE(tokens[11].is(TokenType::AmpAmp));
    EXPECT_TRUE(tokens[12].is(TokenType::PipePipe));
    EXPECT_TRUE(tokens[13].is(TokenType::Exclaim));
    EXPECT_TRUE(tokens[14].is(TokenType::Equal));
    EXPECT_TRUE(tokens[15].is(TokenType::Semicolon));
    EXPECT_TRUE(tokens[16].is(TokenType::Comma));
    EXPECT_TRUE(tokens[17].is(TokenType::LParen));
    EXPECT_TRUE(tokens[18].is(TokenType::RParen));
    EXPECT_TRUE(tokens[19].is(TokenType::LBrace));
    EXPECT_TRUE(tokens[20].is(TokenType::RBrace));
}

TEST(LexerTest, CommentsAndLocationTracking) {
    DiagnosticEngine diag;
    std::string src = 
        "// First line comment\n"
        "/* Multi-line\n"
        "   comment */\n"
        "int a; // trailing comment\n";
    Lexer lexer(src, "test.mc", diag);
    auto tokens = lexer.tokenizeAll();

    EXPECT_FALSE(diag.hasErrors());
    EXPECT_EQ(tokens.size(), 4u); // int, a, ;, EOF
    EXPECT_TRUE(tokens[0].is(TokenType::KwInt));
    EXPECT_EQ(tokens[0].location.line, 4);
    EXPECT_EQ(tokens[0].location.column, 1);

    EXPECT_TRUE(tokens[1].is(TokenType::Identifier));
    EXPECT_EQ(tokens[1].lexeme, "a");
    EXPECT_EQ(tokens[1].location.line, 4);
    EXPECT_EQ(tokens[1].location.column, 5);
}

TEST(LexerTest, UnterminatedCommentError) {
    DiagnosticEngine diag;
    std::string src = "/* unterminated comment";
    Lexer lexer(src, "test.mc", diag);
    auto tokens = lexer.tokenizeAll();

    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.errorCount(), 1u);
}

TEST(LexerTest, InvalidTokens) {
    DiagnosticEngine diag;
    std::string src = "@ # $";
    Lexer lexer(src, "test.mc", diag);
    auto tokens = lexer.tokenizeAll();

    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.errorCount(), 3u);
}
