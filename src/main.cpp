#include "minic/common/Diagnostic.hpp"
#include "minic/lexer/Lexer.hpp"
#include "minic/parser/Parser.hpp"
#include "minic/ast/ASTDumper.hpp"
#include "minic/semantic/SemanticAnalyzer.hpp"
#include "minic/ir/IRBuilder.hpp"
#include "minic/ir/LivenessAnalysis.hpp"
#include "minic/optimizer/PassManager.hpp"
#include "minic/codegen/PTXEmitter.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

void printUsage(const char* progName) {
    std::cout << "MiniC-to-PTX Compiler v0.1.0\n"
              << "Translates MiniC programs into NVIDIA PTX Assembly.\n\n"
              << "Usage: " << progName << " [options] <input-file>\n\n"
              << "Options:\n"
              << "  --help              Display this information\n"
              << "  --version           Display compiler version\n"
              << "  --emit-ast          Emit Abstract Syntax Tree representation\n"
              << "  --emit-ir           Emit Three-Address Code intermediate representation\n"
              << "  --emit-ptx          Emit PTX assembly (default)\n"
              << "  --emit-cfg-dot      Emit Control-Flow Graph in Graphviz DOT format\n"
              << "  --emit-liveness     Emit liveness analysis report and register intervals\n"
              << "  --optimize, -O1     Enable compiler optimization passes\n"
              << "  --target=<type>     Target output type: 'func' (default) or 'kernel'\n"
              << "  --arch=<sm>         Target SM architecture (default: sm_70)\n"
              << "  --validate-ptx      Validate generated PTX using NVIDIA ptxas if available\n"
              << "  -o <file>           Output file (defaults to stdout)\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputFile;
    std::string outputFile;
    std::string targetType = "func";
    std::string arch = "sm_70";
    bool emitAST = false;
    bool emitIR = false;
    bool emitPTX = true;
    bool emitCFGDot = false;
    bool emitLiveness = false;
    bool optimize = false;
    bool validatePTX = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--version" || arg == "-v") {
            std::cout << "MiniC-to-PTX version 0.1.0 (Educational GPU Compiler)\n"
                      << "Targeting NVIDIA PTX ISA 7.0+\n";
            return 0;
        } else if (arg == "--emit-ast") {
            emitAST = true;
            emitPTX = false;
        } else if (arg == "--emit-ir") {
            emitIR = true;
            emitPTX = false;
        } else if (arg == "--emit-ptx") {
            emitPTX = true;
        } else if (arg == "--emit-cfg-dot") {
            emitCFGDot = true;
            emitPTX = false;
        } else if (arg == "--emit-liveness") {
            emitLiveness = true;
            emitPTX = false;
        } else if (arg == "--optimize" || arg == "-O1") {
            optimize = true;
        } else if (arg == "--validate-ptx") {
            validatePTX = true;
        } else if (arg.rfind("--target=", 0) == 0) {
            targetType = arg.substr(9);
        } else if (arg.rfind("--arch=", 0) == 0) {
            arch = arg.substr(7);
        } else if (arg == "-o") {
            if (i + 1 < argc) {
                outputFile = argv[++i];
            } else {
                std::cerr << "Error: -o requires an argument\n";
                return 1;
            }
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Error: Unknown option '" << arg << "'\n";
            printUsage(argv[0]);
            return 1;
        } else {
            inputFile = arg;
        }
    }

    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified\n";
        return 1;
    }

    std::ifstream file(inputFile);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << inputFile << "'\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    (void)emitIR;
    (void)emitPTX;
    (void)emitCFGDot;
    (void)optimize;
    (void)validatePTX;
    (void)targetType;
    (void)arch;

    minic::DiagnosticEngine diag(source, inputFile);
    minic::Lexer lexer(source, inputFile, diag);
    minic::parser::Parser parser(lexer, diag);

    auto program = parser.parseProgram();

    if (diag.hasErrors()) {
        diag.printAll(std::cerr);
        return 1;
    }

    // Run Semantic Analysis
    minic::semantic::SemanticAnalyzer sema(diag);
    if (!sema.analyze(*program)) {
        diag.printAll(std::cerr);
        return 1;
    }

    if (emitAST) {
        if (!outputFile.empty()) {
            std::ofstream out(outputFile);
            if (!out.is_open()) {
                std::cerr << "Error: Could not open output file '" << outputFile << "'\n";
                return 1;
            }
            minic::ast::ASTDumper dumper(out);
            dumper.dump(*program);
        } else {
            minic::ast::ASTDumper dumper(std::cout);
            dumper.dump(*program);
        }
        return 0;
    }

    // Lower AST to Three-Address Code (TAC) IR
    minic::ir::IRBuilder irBuilder(inputFile);
    auto irModule = irBuilder.build(*program);

    if (optimize) {
        auto pm = minic::optimizer::PassManager::createDefaultPipeline();
        pm.run(*irModule);
    }

    if (emitIR) {
        if (!outputFile.empty()) {
            std::ofstream out(outputFile);
            if (!out.is_open()) {
                std::cerr << "Error: Could not open output file '" << outputFile << "'\n";
                return 1;
            }
            irModule->print(out);
        } else {
            irModule->print(std::cout);
        }
        return 0;
    }

    if (emitCFGDot) {
        if (!outputFile.empty()) {
            std::ofstream out(outputFile);
            if (!out.is_open()) {
                std::cerr << "Error: Could not open output file '" << outputFile << "'\n";
                return 1;
            }
            irModule->printCFGDot(out);
        } else {
            irModule->printCFGDot(std::cout);
        }
        return 0;
    }

    if (emitLiveness) {
        std::ostringstream oss;
        for (const auto& fn : irModule->functions()) {
            minic::ir::LivenessAnalysis liveness(*fn);
            liveness.run();
            liveness.printReport(oss);
            oss << "\n";
        }
        if (!outputFile.empty()) {
            std::ofstream out(outputFile);
            if (!out.is_open()) {
                std::cerr << "Error: Could not open output file '" << outputFile << "'\n";
                return 1;
            }
            out << oss.str();
        } else {
            std::cout << oss.str();
        }
        return 0;
    }

    if (emitPTX) {
        bool isKernelTarget = (targetType == "kernel");
        minic::codegen::PTXTarget target(arch, isKernelTarget, "7.0");
        minic::codegen::PTXEmitter emitter(target);

        std::string ptxOutput;
        std::ostringstream oss;
        emitter.emitModule(*irModule, oss);
        ptxOutput = oss.str();

        if (!outputFile.empty()) {
            std::ofstream out(outputFile);
            if (!out.is_open()) {
                std::cerr << "Error: Could not open output file '" << outputFile << "'\n";
                return 1;
            }
            out << ptxOutput;
        } else {
            std::cout << ptxOutput;
        }

        if (validatePTX) {
            std::string tempFile = outputFile.empty() ? "temp_output.ptx" : outputFile;
            if (outputFile.empty()) {
                std::ofstream tempOut(tempFile);
                tempOut << ptxOutput;
            }
            std::string pyCmd = "python scripts/validate_ptx.py " + tempFile + " --arch=" + arch;
            int ret = std::system(pyCmd.c_str());
            if (outputFile.empty() && std::ifstream(tempFile).good()) {
                std::remove(tempFile.c_str());
            }
            if (ret != 0) {
                return 1;
            }
        }
        return 0;
    }

    return 0;
}
