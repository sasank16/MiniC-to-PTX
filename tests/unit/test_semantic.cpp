#include "tests/test_framework.hpp"
#include "minic/lexer/Lexer.hpp"
#include "minic/parser/Parser.hpp"
#include "minic/semantic/SemanticAnalyzer.hpp"

using namespace minic;

static bool runSemanticCheck(const std::string& src, DiagnosticEngine& diag) {
    Lexer lexer(src, "test.mc", diag);
    parser::Parser parser(lexer, diag);
    auto prog = parser.parseProgram();
    if (diag.hasErrors()) {
        return false;
    }
    semantic::SemanticAnalyzer sema(diag);
    return sema.analyze(*prog);
}

TEST(SemanticTest, ValidProgramPasses) {
    DiagnosticEngine diag;
    std::string src = 
        "int add(int a, int b) {\n"
        "    return a + b;\n"
        "}\n"
        "int main() {\n"
        "    int res = add(3, 4);\n"
        "    return res;\n"
        "}\n";
    bool ok = runSemanticCheck(src, diag);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(diag.hasErrors());
    EXPECT_EQ(diag.errorCount(), 0u);
}

TEST(SemanticTest, InnerScopeShadowingAllowed) {
    DiagnosticEngine diag;
    std::string src = 
        "int test() {\n"
        "    int x = 1;\n"
        "    {\n"
        "        int x = 2;\n" // Inner scope shadowing outer x is valid C
        "        x = x + 1;\n"
        "    }\n"
        "    return x;\n"
        "}\n";
    bool ok = runSemanticCheck(src, diag);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(diag.hasErrors());
}

TEST(SemanticTest, UndeclaredVariableDetected) {
    DiagnosticEngine diag;
    std::string src = 
        "int test() {\n"
        "    int y = z + 1;\n" // z is undeclared
        "    return y;\n"
        "}\n";
    bool ok = runSemanticCheck(src, diag);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.errorCount(), 1u);
}

TEST(SemanticTest, DuplicateVariableInSameScopeRejected) {
    DiagnosticEngine diag;
    std::string src = 
        "int test() {\n"
        "    int x = 1;\n"
        "    int x = 2;\n" // Duplicate in same scope
        "    return x;\n"
        "}\n";
    bool ok = runSemanticCheck(src, diag);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.errorCount(), 1u);
}

TEST(SemanticTest, DuplicateParameterNameRejected) {
    DiagnosticEngine diag;
    std::string src = 
        "int test(int a, int a) {\n" // Duplicate param 'a'
        "    return a;\n"
        "}\n";
    bool ok = runSemanticCheck(src, diag);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.errorCount(), 1u);
}

TEST(SemanticTest, StaticDivisionByZeroDetected) {
    DiagnosticEngine diag;
    std::string src = 
        "int test(int x) {\n"
        "    int a = x / 0;\n" // Division by zero
        "    int b = x % 0;\n" // Modulo by zero
        "    return a + b;\n"
        "}\n";
    bool ok = runSemanticCheck(src, diag);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.errorCount(), 2u);
}

TEST(SemanticTest, VoidVariableRejected) {
    DiagnosticEngine diag;
    std::string src = 
        "int test() {\n"
        "    void x;\n" // Illegal variable type
        "    return 0;\n"
        "}\n";
    bool ok = runSemanticCheck(src, diag);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.errorCount(), 1u);
}

TEST(SemanticTest, ReturnValueFromVoidFunctionRejected) {
    DiagnosticEngine diag;
    std::string src = 
        "void proc() {\n"
        "    return 42;\n" // Illegal return value in void function
        "}\n";
    bool ok = runSemanticCheck(src, diag);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.errorCount(), 1u);
}

TEST(SemanticTest, MissingReturnValueInIntFunctionRejected) {
    DiagnosticEngine diag;
    std::string src = 
        "int compute() {\n"
        "    return;\n" // Missing return expression
        "}\n";
    bool ok = runSemanticCheck(src, diag);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.errorCount(), 1u);
}

TEST(SemanticTest, FunctionCallArgumentCountMismatch) {
    DiagnosticEngine diag;
    std::string src = 
        "int add(int a, int b) {\n"
        "    return a + b;\n"
        "}\n"
        "int test() {\n"
        "    return add(1);\n" // Missing 2nd argument
        "}\n";
    bool ok = runSemanticCheck(src, diag);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.errorCount(), 1u);
}

TEST(SemanticTest, CallUndeclaredFunctionRejected) {
    DiagnosticEngine diag;
    std::string src = 
        "int test() {\n"
        "    return unknown_fn(1, 2);\n"
        "}\n";
    bool ok = runSemanticCheck(src, diag);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.errorCount(), 1u);
}
