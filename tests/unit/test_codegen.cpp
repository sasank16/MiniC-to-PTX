#include "tests/test_framework.hpp"
#include "minic/lexer/Lexer.hpp"
#include "minic/parser/Parser.hpp"
#include "minic/semantic/SemanticAnalyzer.hpp"
#include "minic/ir/IRBuilder.hpp"
#include "minic/optimizer/PassManager.hpp"
#include "minic/codegen/PTXEmitter.hpp"
#include <sstream>

using namespace minic;

static std::string compileToPTX(const std::string& src, bool optimize = false, bool isKernel = false) {
    DiagnosticEngine diag;
    Lexer lexer(src, "test.mc", diag);
    parser::Parser parser(lexer, diag);
    auto prog = parser.parseProgram();
    if (diag.hasErrors()) return "";

    semantic::SemanticAnalyzer sema(diag);
    if (!sema.analyze(*prog)) return "";

    ir::IRBuilder builder("test_module");
    auto mod = builder.build(*prog);

    if (optimize) {
        auto pm = optimizer::PassManager::createDefaultPipeline();
        pm.run(*mod);
    }

    codegen::PTXTarget target("sm_70", isKernel, "7.0");
    codegen::PTXEmitter emitter(target);
    std::ostringstream oss;
    emitter.emitModule(*mod, oss);
    return oss.str();
}

TEST(PTXCodegenTest, TargetHeaderDirectives) {
    std::string ptx = compileToPTX("int foo() { return 1; }");
    EXPECT_TRUE(ptx.find(".version 7.0") != std::string::npos);
    EXPECT_TRUE(ptx.find(".target sm_70") != std::string::npos);
    EXPECT_TRUE(ptx.find(".address_size 64") != std::string::npos);
}

TEST(PTXCodegenTest, DeviceFunctionAndArithmetic) {
    std::string src = 
        "int compute(int a, int b) {\n"
        "    return a + b * 2;\n"
        "}\n";
    std::string ptx = compileToPTX(src);

    EXPECT_TRUE(ptx.find(".visible .func (.param .s32 %retval) compute") != std::string::npos);
    EXPECT_TRUE(ptx.find(".param .s32 %param_a") != std::string::npos);
    EXPECT_TRUE(ptx.find(".param .s32 %param_b") != std::string::npos);
    EXPECT_TRUE(ptx.find("ld.param.s32") != std::string::npos);
    EXPECT_TRUE(ptx.find("mul.lo.s32") != std::string::npos);
    EXPECT_TRUE(ptx.find("add.s32") != std::string::npos);
    EXPECT_TRUE(ptx.find("st.param.s32 [%retval]") != std::string::npos);
    EXPECT_TRUE(ptx.find("ret;") != std::string::npos);
}

TEST(PTXCodegenTest, KernelEntryAndGlobalStore) {
    std::string src = 
        "__global__ int gpu_kernel(int x) {\n"
        "    return x + 10;\n"
        "}\n";
    std::string ptx = compileToPTX(src, false, true);

    EXPECT_TRUE(ptx.find(".visible .entry gpu_kernel") != std::string::npos);
    EXPECT_TRUE(ptx.find(".param .u64 %out_ptr") != std::string::npos);
    EXPECT_TRUE(ptx.find(".reg .u64  %rd<1>;") != std::string::npos);
    EXPECT_TRUE(ptx.find("ld.param.u64 %rd0, [%out_ptr];") != std::string::npos);
    EXPECT_TRUE(ptx.find("st.global.s32 [%rd0]") != std::string::npos);
    EXPECT_TRUE(ptx.find("ret;") != std::string::npos);
}

TEST(PTXCodegenTest, PredicatedBranchingAndConditionals) {
    std::string src = 
        "int max_val(int a, int b) {\n"
        "    if (a > b) return a;\n"
        "    return b;\n"
        "}\n";
    std::string ptx = compileToPTX(src);

    EXPECT_TRUE(ptx.find(".reg .pred %p<") != std::string::npos);
    EXPECT_TRUE(ptx.find("setp.gt.s32") != std::string::npos);
    EXPECT_TRUE(ptx.find("@%p") != std::string::npos);
    EXPECT_TRUE(ptx.find("bra") != std::string::npos);
}

TEST(PTXCodegenTest, WhileLoopWithBackBranch) {
    std::string src = 
        "int sum_n(int n) {\n"
        "    int total = 0;\n"
        "    int i = 1;\n"
        "    while (i <= n) {\n"
        "        total = total + i;\n"
        "        i = i + 1;\n"
        "    }\n"
        "    return total;\n"
        "}\n";
    std::string ptx = compileToPTX(src);

    EXPECT_TRUE(ptx.find("setp.le.s32") != std::string::npos);
    EXPECT_TRUE(ptx.find("L_while_cond") != std::string::npos);
    EXPECT_TRUE(ptx.find("L_while_body") != std::string::npos);
    EXPECT_TRUE(ptx.find("L_while_end") != std::string::npos);
}

TEST(PTXCodegenTest, OptimizedPTXReducesInstructionCount) {
    std::string src = 
        "int folded() {\n"
        "    int a = 10 + 20 * 2;\n"
        "    return a;\n"
        "}\n";
    std::string unoptPTX = compileToPTX(src, false);
    std::string optPTX = compileToPTX(src, true);

    // Unoptimized PTX has mul.lo.s32 and add.s32
    EXPECT_TRUE(unoptPTX.find("mul.lo.s32") != std::string::npos);

    // Optimized PTX has folded computation: 50 directly stored into retval
    EXPECT_TRUE(optPTX.find("mul.lo.s32") == std::string::npos);
    EXPECT_TRUE(optPTX.find("50") != std::string::npos);
}
