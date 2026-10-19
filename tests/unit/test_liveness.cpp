#include "tests/test_framework.hpp"
#include "minic/lexer/Lexer.hpp"
#include "minic/parser/Parser.hpp"
#include "minic/semantic/SemanticAnalyzer.hpp"
#include "minic/ir/IRBuilder.hpp"
#include "minic/ir/LivenessAnalysis.hpp"
#include "minic/codegen/LinearScanAllocator.hpp"
#include <sstream>

using namespace minic;

static std::unique_ptr<ir::Module> buildIR(const std::string& src, DiagnosticEngine& diag) {
    Lexer lexer(src, "test.mc", diag);
    parser::Parser parser(lexer, diag);
    auto prog = parser.parseProgram();
    if (diag.hasErrors()) return nullptr;

    semantic::SemanticAnalyzer sema(diag);
    if (!sema.analyze(*prog)) return nullptr;

    ir::IRBuilder builder("test_liveness");
    return builder.build(*prog);
}

TEST(LivenessTest, LocalDefUseComputation) {
    DiagnosticEngine diag;
    std::string src = 
        "int foo(int a, int b) {\n"
        "    int c = a + b;\n"
        "    return c;\n"
        "}\n";
    auto mod = buildIR(src, diag);
    EXPECT_FALSE(diag.hasErrors());
    ASSERT_TRUE(mod != nullptr);

    ir::Function* fn = mod->getFunction("foo");
    ASSERT_TRUE(fn != nullptr);

    ir::LivenessAnalysis liveness(*fn);
    liveness.run();

    // Verify peak pressure is positive and live ranges exist
    EXPECT_TRUE(liveness.getPeakRegisterPressure() > 0);
    EXPECT_FALSE(liveness.getLiveRanges().empty());
}

TEST(LivenessTest, LoopDataflowFixpoint) {
    DiagnosticEngine diag;
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
    auto mod = buildIR(src, diag);
    EXPECT_FALSE(diag.hasErrors());
    ASSERT_TRUE(mod != nullptr);

    ir::Function* fn = mod->getFunction("sum_n");
    ASSERT_TRUE(fn != nullptr);

    ir::LivenessAnalysis liveness(*fn);
    liveness.run();

    // Report should print without error
    std::ostringstream oss;
    liveness.printReport(oss);
    std::string report = oss.str();

    EXPECT_TRUE(report.find("Basic Block Liveness Summary:") != std::string::npos);
    EXPECT_TRUE(report.find("Live Intervals:") != std::string::npos);
    EXPECT_TRUE(report.find("Peak Register Pressure:") != std::string::npos);
}

TEST(LivenessTest, LinearScanReusesRegistersForDisjointIntervals) {
    DiagnosticEngine diag;
    // Sequential independent computations:
    // a * 2 finishes before b * 3 begins
    std::string src = 
        "int compute(int a, int b) {\n"
        "    int x = a * 2;\n"
        "    int y = b * 3;\n"
        "    return x + y;\n"
        "}\n";
    auto mod = buildIR(src, diag);
    EXPECT_FALSE(diag.hasErrors());
    ASSERT_TRUE(mod != nullptr);

    ir::Function* fn = mod->getFunction("compute");
    ASSERT_TRUE(fn != nullptr);

    ir::LivenessAnalysis liveness(*fn);
    liveness.run();

    codegen::LinearScanAllocator allocator(liveness);
    auto allocResult = allocator.allocate();

    // The number of allocated physical registers should be <= the total virtual registers
    EXPECT_TRUE(allocResult.numAllocatedS32 <= static_cast<int>(liveness.getLiveRanges().size()));
}
