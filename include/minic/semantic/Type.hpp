#pragma once

#include "minic/ast/AST.hpp"
#include <string>

namespace minic::semantic {

enum class TypeKind {
    Int,
    Void,
    Error
};

class Type {
public:
    explicit Type(TypeKind kind) : kind_(kind) {}

    static Type getInt() { return Type(TypeKind::Int); }
    static Type getVoid() { return Type(TypeKind::Void); }
    static Type getError() { return Type(TypeKind::Error); }

    static Type fromAST(ast::TypeKind astKind) {
        switch (astKind) {
            case ast::TypeKind::Int:  return getInt();
            case ast::TypeKind::Void: return getVoid();
        }
        return getError();
    }

    TypeKind kind() const { return kind_; }
    bool isInt() const { return kind_ == TypeKind::Int; }
    bool isVoid() const { return kind_ == TypeKind::Void; }
    bool isError() const { return kind_ == TypeKind::Error; }

    bool operator==(const Type& other) const { return kind_ == other.kind_; }
    bool operator!=(const Type& other) const { return !(*this == other); }

    std::string toString() const {
        switch (kind_) {
            case TypeKind::Int:   return "int";
            case TypeKind::Void:  return "void";
            case TypeKind::Error: return "<error-type>";
        }
        return "<unknown>";
    }

private:
    TypeKind kind_;
};

} // namespace minic::semantic
