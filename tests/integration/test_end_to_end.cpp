#include "tests/test_framework.hpp"
#include "minic/common/Diagnostic.hpp"
#include "minic/lexer/Lexer.hpp"
#include "minic/parser/Parser.hpp"
#include "minic/ast/ASTDumper.hpp"
#include "minic/semantic/SemanticAnalyzer.hpp"
#include "minic/ir/IRBuilder.hpp"
#include "minic/ir/LivenessAnalysis.hpp"
#include "minic/optimizer/PassManager.hpp"
#include "minic/codegen/PTXEmitter.hpp"
#include <fstream>
#include <sstream>

using namespace minic;

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        f.open("../" + path);
    }
    if (!f.is_open()) {
        f.open("../../" + path);
    }
    if (!f.is_open()) return "";
    std::stringstream buf;
    buf << f.rdbuf();
    return buf.str();
}

static bool compileFullPipeline(const std::string& filePath, bool optimize, bool isKernel,
                                std::string* outAST = nullptr,
                                std::string* outIR = nullptr,
                                std::string* outLiveness = nullptr,
                                std::string* outPTX = nullptr) {
    std::string src = readFile(filePath);
    if (src.empty()) return false;

    DiagnosticEngine diag(src, filePath);
    Lexer lexer(src, filePath, diag);
    parser::Parser parser(lexer, diag);
    auto prog = parser.parseProgram();
    if (diag.hasErrors() || !prog) return false;

    if (outAST) {
        std::ostringstream oss;
        ast::ASTDumper dumper(oss);
        dumper.dump(*prog);
        *outAST = oss.str();
    }

    semantic::SemanticAnalyzer sema(diag);
    if (!sema.analyze(*prog)) return false;

    ir::IRBuilder builder(filePath);
    auto mod = builder.build(*prog);
    if (!mod) return false;

    if (optimize) {
        auto pm = optimizer::PassManager::createDefaultPipeline();
        pm.run(*mod);
    }

    if (outIR) {
        std::ostringstream oss;
        mod->print(oss);
        *outIR = oss.str();
    }

    if (outLiveness) {
        std::ostringstream oss;
        for (const auto& fn : mod->functions()) {
            ir::LivenessAnalysis liveness(*fn);
            liveness.run();
            liveness.printReport(oss);
        }
        *outLiveness = oss.str();
    }

    if (outPTX) {
        codegen::PTXTarget target("sm_70", isKernel, "7.0");
        codegen::PTXEmitter emitter(target);
        std::ostringstream oss;
        emitter.emitModule(*mod, oss);
        *outPTX = oss.str();
    }

    return true;
}

TEST(EndToEndTest, CompileArithmeticExample) {
    std::string ast, ir, liveness, ptx;
    bool ok = compileFullPipeline("examples/arithmetic.mc", false, false, &ast, &ir, &liveness, &ptx);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(ast.find("FunctionDecl: int compute") != std::string::npos);
    EXPECT_TRUE(ir.find("define int @compute") != std::string::npos);
    EXPECT_TRUE(liveness.find("Peak Register Pressure:") != std::string::npos);
    EXPECT_TRUE(ptx.find(".visible .func (.param .s32 %retval) compute") != std::string::npos);
    EXPECT_TRUE(ptx.find("mul.lo.s32") != std::string::npos);
    EXPECT_TRUE(ptx.find("add.s32") != std::string::npos);
}

TEST(EndToEndTest, CompileConditionalExample) {
    std::string ast, ir, liveness, ptx;
    bool ok = compileFullPipeline("examples/conditional.mc", false, false, &ast, &ir, &liveness, &ptx);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(ast.find("IfStmt") != std::string::npos);
    EXPECT_TRUE(ir.find("br_cond") != std::string::npos);
    EXPECT_TRUE(ptx.find("setp.gt.s32") != std::string::npos);
    EXPECT_TRUE(ptx.find("@%p") != std::string::npos);
}

TEST(EndToEndTest, CompileLoopExample) {
    std::string ast, ir, liveness, ptx;
    bool ok = compileFullPipeline("examples/loop.mc", false, false, &ast, &ir, &liveness, &ptx);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(ast.find("WhileStmt") != std::string::npos);
    EXPECT_TRUE(ir.find("while.cond") != std::string::npos);
    EXPECT_TRUE(ptx.find("L_while_cond") != std::string::npos);
    EXPECT_TRUE(ptx.find("L_while_body") != std::string::npos);
}

TEST(EndToEndTest, CompileOptimizationExample) {
    std::string unoptIR, optIR, optPTX;
    bool ok1 = compileFullPipeline("examples/optimization.mc", false, false, nullptr, &unoptIR, nullptr, nullptr);
    bool ok2 = compileFullPipeline("examples/optimization.mc", true, false, nullptr, &optIR, nullptr, &optPTX);

    EXPECT_TRUE(ok1);
    EXPECT_TRUE(ok2);

    // Constant folded in optimized IR
    EXPECT_TRUE(optIR.find("250") != std::string::npos);

    // Dead code eliminated in optimized IR
    EXPECT_TRUE(unoptIR.find("999") != std::string::npos);
    EXPECT_TRUE(optIR.find("999") == std::string::npos);

    // PTX should have 250 and no 999
    EXPECT_TRUE(optPTX.find("250") != std::string::npos);
    EXPECT_TRUE(optPTX.find("999") == std::string::npos);
}

TEST(EndToEndTest, CompileVectorComputeKernel) {
    std::string ast, ir, liveness, ptx;
    bool ok = compileFullPipeline("examples/vector_add.mc", true, true, &ast, &ir, &liveness, &ptx);
    EXPECT_TRUE(ok);

    EXPECT_TRUE(ast.find("FunctionDecl: int vector_compute [__global__]") != std::string::npos);
    EXPECT_TRUE(ir.find("kernel int @vector_compute") != std::string::npos);
    EXPECT_TRUE(ptx.find(".visible .entry vector_compute") != std::string::npos);
    EXPECT_TRUE(ptx.find(".param .u64 %out_ptr") != std::string::npos);
    EXPECT_TRUE(ptx.find("st.global.s32") != std::string::npos);
}
