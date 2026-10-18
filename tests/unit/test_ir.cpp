#include "tests/test_framework.hpp"
#include "minic/lexer/Lexer.hpp"
#include "minic/parser/Parser.hpp"
#include "minic/semantic/SemanticAnalyzer.hpp"
#include "minic/ir/IRBuilder.hpp"
#include <sstream>

using namespace minic;

static std::unique_ptr<ir::Module> compileToIR(const std::string& src, DiagnosticEngine& diag) {
    Lexer lexer(src, "test.mc", diag);
    parser::Parser parser(lexer, diag);
    auto prog = parser.parseProgram();
    if (diag.hasErrors()) return nullptr;

    semantic::SemanticAnalyzer sema(diag);
    if (!sema.analyze(*prog)) return nullptr;

    ir::IRBuilder builder("test_module");
    return builder.build(*prog);
}

TEST(IRTest, ArithmeticLowersCorrectly) {
    DiagnosticEngine diag;
    std::string src = 
        "int compute(int a, int b) {\n"
        "    return a + b * 2;\n"
        "}\n";
    auto mod = compileToIR(src, diag);
    EXPECT_FALSE(diag.hasErrors());
    ASSERT_TRUE(mod != nullptr);

    ir::Function* fn = mod->getFunction("compute");
    ASSERT_TRUE(fn != nullptr);
    EXPECT_EQ(fn->params().size(), 2u);

    std::ostringstream oss;
    mod->print(oss);
    std::string irStr = oss.str();

    EXPECT_TRUE(irStr.find("mul") != std::string::npos);
    EXPECT_TRUE(irStr.find("add") != std::string::npos);
    EXPECT_TRUE(irStr.find("ret") != std::string::npos);
}

TEST(IRTest, VariableStoreAndLoad) {
    DiagnosticEngine diag;
    std::string src = 
        "int test() {\n"
        "    int x = 10;\n"
        "    x = x + 5;\n"
        "    return x;\n"
        "}\n";
    auto mod = compileToIR(src, diag);
    EXPECT_FALSE(diag.hasErrors());
    ASSERT_TRUE(mod != nullptr);

    std::ostringstream oss;
    mod->print(oss);
    std::string irStr = oss.str();

    EXPECT_TRUE(irStr.find("%x = alloca int") != std::string::npos);
    EXPECT_TRUE(irStr.find("store 10, %x") != std::string::npos);
    EXPECT_TRUE(irStr.find("load %x") != std::string::npos);
}

TEST(IRTest, IfElseControlFlowAndEdges) {
    DiagnosticEngine diag;
    std::string src = 
        "int max_val(int a, int b) {\n"
        "    if (a > b) {\n"
        "        return a;\n"
        "    } else {\n"
        "        return b;\n"
        "    }\n"
        "}\n";
    auto mod = compileToIR(src, diag);
    EXPECT_FALSE(diag.hasErrors());
    ASSERT_TRUE(mod != nullptr);

    ir::Function* fn = mod->getFunction("max_val");
    ASSERT_TRUE(fn != nullptr);

    // Expect multiple basic blocks: entry, if.then, if.else, if.end
    EXPECT_TRUE(fn->blocks().size() >= 3u);

    ir::BasicBlock* entry = fn->entryBlock();
    ASSERT_TRUE(entry != nullptr);
    EXPECT_EQ(entry->successors().size(), 2u); // branches to then and else

    std::ostringstream oss;
    mod->print(oss);
    std::string irStr = oss.str();
    EXPECT_TRUE(irStr.find("br_cond") != std::string::npos);
    EXPECT_TRUE(irStr.find("cmp_gt") != std::string::npos);
}

TEST(IRTest, WhileLoopControlFlowAndBackEdge) {
    DiagnosticEngine diag;
    std::string src = 
        "int count_down(int n) {\n"
        "    while (n > 0) {\n"
        "        n = n - 1;\n"
        "    }\n"
        "    return n;\n"
        "}\n";
    auto mod = compileToIR(src, diag);
    EXPECT_FALSE(diag.hasErrors());
    ASSERT_TRUE(mod != nullptr);

    ir::Function* fn = mod->getFunction("count_down");
    ASSERT_TRUE(fn != nullptr);

    std::ostringstream oss;
    mod->print(oss);
    std::string irStr = oss.str();

    // Verify while.cond and while.body exist and have branches
    EXPECT_TRUE(irStr.find("while.cond") != std::string::npos);
    EXPECT_TRUE(irStr.find("while.body") != std::string::npos);
    EXPECT_TRUE(irStr.find("while.end") != std::string::npos);

    // Verify CFG DOT visualization output
    std::ostringstream dot;
    mod->printCFGDot(dot);
    std::string dotStr = dot.str();
    EXPECT_TRUE(dotStr.find("digraph \"count_down\"") != std::string::npos);
    EXPECT_TRUE(dotStr.find("->") != std::string::npos);
}

TEST(IRTest, FunctionCallLowersToCallInst) {
    DiagnosticEngine diag;
    std::string src = 
        "int square(int x) { return x * x; }\n"
        "int main() {\n"
        "    int y = square(5);\n"
        "    return y;\n"
        "}\n";
    auto mod = compileToIR(src, diag);
    EXPECT_FALSE(diag.hasErrors());
    ASSERT_TRUE(mod != nullptr);

    std::ostringstream oss;
    mod->print(oss);
    std::string irStr = oss.str();
    EXPECT_TRUE(irStr.find("call @square") != std::string::npos);
}
