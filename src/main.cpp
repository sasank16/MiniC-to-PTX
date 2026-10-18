#include "minic/common/Diagnostic.hpp"
#include "minic/lexer/Lexer.hpp"
#include "minic/parser/Parser.hpp"
#include "minic/ast/ASTDumper.hpp"
#include "minic/semantic/SemanticAnalyzer.hpp"
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

    std::cout << "// Parsed and verified program successfully with " 
              << program->functions().size() << " function(s).\n";

    return 0;
}
