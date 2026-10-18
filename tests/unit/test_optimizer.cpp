#include "tests/test_framework.hpp"
#include "minic/lexer/Lexer.hpp"
#include "minic/parser/Parser.hpp"
#include "minic/semantic/SemanticAnalyzer.hpp"
#include "minic/ir/IRBuilder.hpp"
#include "minic/optimizer/PassManager.hpp"
#include "minic/optimizer/ConstantFoldingPass.hpp"
#include "minic/optimizer/DeadCodeEliminationPass.hpp"
#include "minic/optimizer/CSEPass.hpp"
#include <sstream>

using namespace minic;

static std::unique_ptr<ir::Module> compileSource(const std::string& src, DiagnosticEngine& diag) {
    Lexer lexer(src, "test.mc", diag);
    parser::Parser parser(lexer, diag);
    auto prog = parser.parseProgram();
    if (diag.hasErrors()) return nullptr;

    semantic::SemanticAnalyzer sema(diag);
    if (!sema.analyze(*prog)) return nullptr;

    ir::IRBuilder builder("opt_test");
    return builder.build(*prog);
}

TEST(OptimizerTest, ConstantFoldingDirectCalculation) {
    DiagnosticEngine diag;
    std::string src = 
        "int calc() {\n"
        "    int a = 10 + 20 * 2;\n" // 10 + 40 = 50
        "    return a;\n"
        "}\n";
    auto mod = compileSource(src, diag);
    EXPECT_FALSE(diag.hasErrors());
    ASSERT_TRUE(mod != nullptr);

    optimizer::ConstantFoldingPass foldPass;
    bool changed = foldPass.runOnModule(*mod);
    EXPECT_TRUE(changed);

    optimizer::DeadCodeEliminationPass dcePass;
    dcePass.runOnModule(*mod);

    std::ostringstream oss;
    mod->print(oss);
    std::string irStr = oss.str();

    // The return statement should now return 50 directly
    EXPECT_TRUE(irStr.find("ret 50") != std::string::npos);
}

TEST(OptimizerTest, AlgebraicIdentityMultiplicationByZero) {
    DiagnosticEngine diag;
    std::string src = 
        "int test(int x) {\n"
        "    int y = x * 0;\n"
        "    return y;\n"
        "}\n";
    auto mod = compileSource(src, diag);
    EXPECT_FALSE(diag.hasErrors());
    ASSERT_TRUE(mod != nullptr);

    optimizer::PassManager pm = optimizer::PassManager::createDefaultPipeline();
    pm.run(*mod);

    std::ostringstream oss;
    mod->print(oss);
    std::string irStr = oss.str();

    EXPECT_TRUE(irStr.find("ret 0") != std::string::npos);
}

TEST(OptimizerTest, DeadCodeEliminationPrunesUnusedMath) {
    DiagnosticEngine diag;
    std::string src = 
        "int test(int x) {\n"
        "    int unused = x * 999 + 42;\n" // Never used
        "    return x;\n"
        "}\n";
    auto mod = compileSource(src, diag);
    EXPECT_FALSE(diag.hasErrors());
    ASSERT_TRUE(mod != nullptr);

    optimizer::DeadCodeEliminationPass dcePass;
    bool changed = dcePass.runOnFunction(*mod->getFunction("test"));
    EXPECT_TRUE(changed);

    std::ostringstream oss;
    mod->print(oss);
    std::string irStr = oss.str();

    // The multiplication by 999 should be eliminated
    EXPECT_TRUE(irStr.find("999") == std::string::npos);
}

TEST(OptimizerTest, ConstantBranchEliminatesUnreachableBlock) {
    DiagnosticEngine diag;
    std::string src = 
        "int test() {\n"
        "    int a = 0;\n"
        "    if (1) {\n"
        "        a = 42;\n"
        "    } else {\n"
        "        a = 99;\n"
        "    }\n"
        "    return a;\n"
        "}\n";
    auto mod = compileSource(src, diag);
    EXPECT_FALSE(diag.hasErrors());
    ASSERT_TRUE(mod != nullptr);

    ir::Function* fn = mod->getFunction("test");
    size_t initialBlockCount = fn->blocks().size();

    optimizer::PassManager pm = optimizer::PassManager::createDefaultPipeline();
    pm.run(*mod);

    // Unreachable else block should be eliminated
    EXPECT_TRUE(fn->blocks().size() < initialBlockCount);

    std::ostringstream oss;
    mod->print(oss);
    std::string irStr = oss.str();

    // 99 should never appear in the optimized IR
    EXPECT_TRUE(irStr.find("99") == std::string::npos);
    EXPECT_TRUE(irStr.find("ret 42") != std::string::npos);
}

TEST(OptimizerTest, CommonSubexpressionElimination) {
    DiagnosticEngine diag;
    std::string src = 
        "int test(int a, int b) {\n"
        "    int x = a + b;\n"
        "    int y = a + b;\n" // Redundant calculation
        "    return x + y;\n"
        "}\n";
    auto mod = compileSource(src, diag);
    EXPECT_FALSE(diag.hasErrors());
    ASSERT_TRUE(mod != nullptr);

    optimizer::PassManager pm = optimizer::PassManager::createDefaultPipeline();
    bool changed = pm.run(*mod);
    EXPECT_TRUE(changed);

    std::ostringstream oss;
    mod->print(oss);
    std::string irStr = oss.str();

    // In the basic block, there should only be one add of %a and %b
    size_t pos = 0;
    int addCount = 0;
    while ((pos = irStr.find("add ", pos)) != std::string::npos) {
        addCount++;
        pos += 4;
    }
    // Originally 3 adds (a+b, a+b, x+y). With CSE + DCE, should be 2 adds!
    EXPECT_EQ(addCount, 2);
}
