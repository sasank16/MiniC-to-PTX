#include "tests/test_framework.hpp"
#include "minic/lexer/Lexer.hpp"
#include "minic/parser/Parser.hpp"
#include "minic/ast/ASTDumper.hpp"
#include <sstream>

using namespace minic;

TEST(ParserTest, SimpleFunctionAndReturn) {
    DiagnosticEngine diag;
    std::string src = "int main() { return 42; }";
    Lexer lexer(src, "test.mc", diag);
    parser::Parser parser(lexer, diag);
    auto prog = parser.parseProgram();

    EXPECT_FALSE(diag.hasErrors());
    ASSERT_TRUE(prog != nullptr);
    ASSERT_EQ(prog->functions().size(), 1u);

    const auto& fn = prog->functions()[0];
    EXPECT_EQ(fn->name(), "main");
    EXPECT_TRUE(fn->returnType() == ast::TypeKind::Int);
    EXPECT_FALSE(fn->isKernel());
    ASSERT_TRUE(fn->body() != nullptr);
    ASSERT_EQ(fn->body()->statements().size(), 1u);

    auto* retStmt = dynamic_cast<ast::ReturnStmt*>(fn->body()->statements()[0].get());
    ASSERT_TRUE(retStmt != nullptr);
    ASSERT_TRUE(retStmt->value() != nullptr);

    auto* lit = dynamic_cast<ast::LiteralExpr*>(retStmt->value());
    ASSERT_TRUE(lit != nullptr);
    EXPECT_EQ(lit->value(), 42);
}

TEST(ParserTest, OperatorPrecedence) {
    DiagnosticEngine diag;
    // a + b * c should parse as a + (b * c)
    std::string src = "int test() { return a + b * c; }";
    Lexer lexer(src, "test.mc", diag);
    parser::Parser parser(lexer, diag);
    auto prog = parser.parseProgram();

    EXPECT_FALSE(diag.hasErrors());
    const auto& fn = prog->functions()[0];
    auto* ret = dynamic_cast<ast::ReturnStmt*>(fn->body()->statements()[0].get());
    ASSERT_TRUE(ret != nullptr);

    auto* topBin = dynamic_cast<ast::BinaryExpr*>(ret->value());
    ASSERT_TRUE(topBin != nullptr);
    EXPECT_TRUE(topBin->op() == ast::BinaryOp::Add);

    // Left should be 'a'
    auto* leftVar = dynamic_cast<ast::VarExpr*>(topBin->left());
    ASSERT_TRUE(leftVar != nullptr);
    EXPECT_EQ(leftVar->name(), "a");

    // Right should be 'b * c'
    auto* rightBin = dynamic_cast<ast::BinaryExpr*>(topBin->right());
    ASSERT_TRUE(rightBin != nullptr);
    EXPECT_TRUE(rightBin->op() == ast::BinaryOp::Mul);
}

TEST(ParserTest, ParenthesizedPrecedence) {
    DiagnosticEngine diag;
    // (a + b) * c should parse as (a + b) * c
    std::string src = "int test() { return (a + b) * c; }";
    Lexer lexer(src, "test.mc", diag);
    parser::Parser parser(lexer, diag);
    auto prog = parser.parseProgram();

    EXPECT_FALSE(diag.hasErrors());
    const auto& fn = prog->functions()[0];
    auto* ret = dynamic_cast<ast::ReturnStmt*>(fn->body()->statements()[0].get());
    ASSERT_TRUE(ret != nullptr);

    auto* topBin = dynamic_cast<ast::BinaryExpr*>(ret->value());
    ASSERT_TRUE(topBin != nullptr);
    EXPECT_TRUE(topBin->op() == ast::BinaryOp::Mul);

    // Left should be 'a + b'
    auto* leftBin = dynamic_cast<ast::BinaryExpr*>(topBin->left());
    ASSERT_TRUE(leftBin != nullptr);
    EXPECT_TRUE(leftBin->op() == ast::BinaryOp::Add);
}

TEST(ParserTest, IfElseAndWhile) {
    DiagnosticEngine diag;
    std::string src = 
        "int foo(int x) {\n"
        "    if (x > 0) {\n"
        "        x = x - 1;\n"
        "    } else {\n"
        "        x = 0;\n"
        "    }\n"
        "    while (x < 10) {\n"
        "        x = x + 1;\n"
        "    }\n"
        "    return x;\n"
        "}\n";
    Lexer lexer(src, "test.mc", diag);
    parser::Parser parser(lexer, diag);
    auto prog = parser.parseProgram();

    EXPECT_FALSE(diag.hasErrors());
    ASSERT_TRUE(prog != nullptr);
    ASSERT_EQ(prog->functions().size(), 1u);
    const auto& fn = prog->functions()[0];
    EXPECT_EQ(fn->params().size(), 1u);
    EXPECT_EQ(fn->params()[0]->name(), "x");
    EXPECT_EQ(fn->body()->statements().size(), 3u); // IfStmt, WhileStmt, ReturnStmt

    auto* ifStmt = dynamic_cast<ast::IfStmt*>(fn->body()->statements()[0].get());
    ASSERT_TRUE(ifStmt != nullptr);
    ASSERT_TRUE(ifStmt->elseBranch() != nullptr);

    auto* whileStmt = dynamic_cast<ast::WhileStmt*>(fn->body()->statements()[1].get());
    ASSERT_TRUE(whileStmt != nullptr);
}

TEST(ParserTest, KernelAttribute) {
    DiagnosticEngine diag;
    std::string src = "__global__ void my_kernel(int n) { return; }";
    Lexer lexer(src, "test.mc", diag);
    parser::Parser parser(lexer, diag);
    auto prog = parser.parseProgram();

    EXPECT_FALSE(diag.hasErrors());
    const auto& fn = prog->functions()[0];
    EXPECT_TRUE(fn->isKernel());
    EXPECT_TRUE(fn->returnType() == ast::TypeKind::Void);
    EXPECT_EQ(fn->name(), "my_kernel");
}

TEST(ParserTest, ASTDumperOutput) {
    DiagnosticEngine diag;
    std::string src = "int calc(int a) { int b = a * 2; return b; }";
    Lexer lexer(src, "test.mc", diag);
    parser::Parser parser(lexer, diag);
    auto prog = parser.parseProgram();

    EXPECT_FALSE(diag.hasErrors());
    std::ostringstream oss;
    ast::ASTDumper dumper(oss);
    dumper.dump(*prog);

    std::string out = oss.str();
    EXPECT_TRUE(out.find("FunctionDecl: int calc") != std::string::npos);
    EXPECT_TRUE(out.find("Param: int a") != std::string::npos);
    EXPECT_TRUE(out.find("VarDeclStmt: int b") != std::string::npos);
    EXPECT_TRUE(out.find("BinaryExpr: '*'") != std::string::npos);
}

TEST(ParserTest, SyntaxErrorRecovery) {
    DiagnosticEngine diag;
    // Missing semicolon after declaration
    std::string src = "int bad() { int x = 5 return x; }";
    Lexer lexer(src, "test.mc", diag);
    parser::Parser parser(lexer, diag);
    auto prog = parser.parseProgram();

    EXPECT_TRUE(diag.hasErrors());
    EXPECT_TRUE(diag.errorCount() >= 1u);
}
