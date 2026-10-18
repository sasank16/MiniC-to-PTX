cmake_minimum_required(VERSION 3.15)
project(MiniC-to-PTX VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Warning flags
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Core compiler library sources
set(MINIC_LIB_SOURCES
    src/common/Diagnostic.cpp
    src/lexer/Lexer.cpp
    src/ast/ASTDumper.cpp
    src/parser/Parser.cpp
    src/semantic/SemanticAnalyzer.cpp
    src/ir/Instruction.cpp
    src/ir/IRBuilder.cpp
)

add_library(minic_lib STATIC ${MINIC_LIB_SOURCES})
target_include_directories(minic_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# CLI compiler binary
add_executable(minic src/main.cpp)
target_link_libraries(minic PRIVATE minic_lib)

# Testing
enable_testing()

add_executable(test_sanity tests/unit/test_framework_sanity.cpp)
target_link_libraries(test_sanity PRIVATE minic_lib)
add_test(NAME SanityTests COMMAND test_sanity)

add_executable(test_lexer tests/unit/test_lexer.cpp tests/test_runner.cpp)
target_link_libraries(test_lexer PRIVATE minic_lib)
add_test(NAME LexerTests COMMAND test_lexer)

add_executable(test_parser tests/unit/test_parser.cpp tests/test_runner.cpp)
target_link_libraries(test_parser PRIVATE minic_lib)
add_test(NAME ParserTests COMMAND test_parser)

add_executable(test_semantic tests/unit/test_semantic.cpp tests/test_runner.cpp)
target_link_libraries(test_semantic PRIVATE minic_lib)
add_test(NAME SemanticTests COMMAND test_semantic)

add_executable(test_ir tests/unit/test_ir.cpp tests/test_runner.cpp)
target_link_libraries(test_ir PRIVATE minic_lib)
add_test(NAME IRTests COMMAND test_ir)
