#pragma once

#include "minic/semantic/Type.hpp"
#include "minic/common/SourceLocation.hpp"
#include <string>
#include <vector>

namespace minic::semantic {

enum class SymbolKind {
    Variable,
    Parameter,
    Function
};

inline const char* symbolKindToString(SymbolKind kind) {
    switch (kind) {
        case SymbolKind::Variable:  return "variable";
        case SymbolKind::Parameter: return "parameter";
        case SymbolKind::Function:  return "function";
    }
    return "symbol";
}

struct Symbol {
    std::string name;
    Type type;
    SymbolKind kind;
    SourceLocation location;

    // For functions
    std::vector<Type> paramTypes;
    bool isKernel{false};

    Symbol() : type(Type::getError()), kind(SymbolKind::Variable) {}
    Symbol(std::string n, Type t, SymbolKind k, SourceLocation loc)
        : name(std::move(n)), type(t), kind(k), location(std::move(loc)) {}

    static Symbol makeVariable(std::string name, Type type, SourceLocation loc) {
        return Symbol(std::move(name), type, SymbolKind::Variable, std::move(loc));
    }

    static Symbol makeParameter(std::string name, Type type, SourceLocation loc) {
        return Symbol(std::move(name), type, SymbolKind::Parameter, std::move(loc));
    }

    static Symbol makeFunction(std::string name, Type retType, std::vector<Type> params, bool isKernel, SourceLocation loc) {
        Symbol sym(std::move(name), retType, SymbolKind::Function, std::move(loc));
        sym.paramTypes = std::move(params);
        sym.isKernel = isKernel;
        return sym;
    }
};

} // namespace minic::semantic
