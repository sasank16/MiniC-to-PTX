#include "minic/ast/ASTDumper.hpp"

namespace minic::ast {

void ASTDumper::indent() {
    for (int i = 0; i < indentLevel_; ++i) {
        out_ << "  ";
    }
}

void ASTDumper::dump(ASTNode& node) {
    node.accept(*this);
}

void ASTDumper::visit(Program& node) {
    out_ << "Program [" << node.location().toString() << "]\n";
    indentLevel_++;
    for (const auto& fn : node.functions()) {
        fn->accept(*this);
    }
    indentLevel_--;
}

void ASTDumper::visit(FunctionDecl& node) {
    indent();
    out_ << "FunctionDecl: " << typeKindToString(node.returnType())
         << " " << node.name()
         << (node.isKernel() ? " [__global__]" : "")
         << " [" << node.location().toString() << "]\n";

    indentLevel_++;
    if (!node.params().empty()) {
        indent();
        out_ << "Parameters:\n";
        indentLevel_++;
        for (const auto& p : node.params()) {
            p->accept(*this);
        }
        indentLevel_--;
    }

    if (node.body()) {
        node.body()->accept(*this);
    }
    indentLevel_--;
}

void ASTDumper::visit(ParamDecl& node) {
    indent();
    out_ << "Param: " << typeKindToString(node.type()) << " " << node.name()
         << " [" << node.location().toString() << "]\n";
}

void ASTDumper::visit(CompoundStmt& node) {
    indent();
    out_ << "CompoundStmt [" << node.location().toString() << "]\n";
    indentLevel_++;
    for (const auto& stmt : node.statements()) {
        stmt->accept(*this);
    }
    indentLevel_--;
}

void ASTDumper::visit(VarDeclStmt& node) {
    indent();
    out_ << "VarDeclStmt: " << typeKindToString(node.type()) << " " << node.name()
         << " [" << node.location().toString() << "]\n";
    if (node.init()) {
        indentLevel_++;
        indent();
        out_ << "Init:\n";
        indentLevel_++;
        node.init()->accept(*this);
        indentLevel_ -= 2;
    }
}

void ASTDumper::visit(ExprStmt& node) {
    indent();
    out_ << "ExprStmt [" << node.location().toString() << "]\n";
    if (node.expr()) {
        indentLevel_++;
        node.expr()->accept(*this);
        indentLevel_--;
    }
}

void ASTDumper::visit(IfStmt& node) {
    indent();
    out_ << "IfStmt [" << node.location().toString() << "]\n";
    indentLevel_++;

    indent();
    out_ << "Condition:\n";
    indentLevel_++;
    node.condition()->accept(*this);
    indentLevel_--;

    indent();
    out_ << "Then:\n";
    indentLevel_++;
    node.thenBranch()->accept(*this);
    indentLevel_--;

    if (node.elseBranch()) {
        indent();
        out_ << "Else:\n";
        indentLevel_++;
        node.elseBranch()->accept(*this);
        indentLevel_--;
    }
    indentLevel_--;
}

void ASTDumper::visit(WhileStmt& node) {
    indent();
    out_ << "WhileStmt [" << node.location().toString() << "]\n";
    indentLevel_++;

    indent();
    out_ << "Condition:\n";
    indentLevel_++;
    node.condition()->accept(*this);
    indentLevel_--;

    indent();
    out_ << "Body:\n";
    indentLevel_++;
    node.body()->accept(*this);
    indentLevel_--;

    indentLevel_--;
}

void ASTDumper::visit(ReturnStmt& node) {
    indent();
    out_ << "ReturnStmt [" << node.location().toString() << "]\n";
    if (node.value()) {
        indentLevel_++;
        node.value()->accept(*this);
        indentLevel_--;
    }
}

void ASTDumper::visit(BinaryExpr& node) {
    indent();
    out_ << "BinaryExpr: '" << binaryOpToString(node.op())
         << "' [" << node.location().toString() << "]\n";
    indentLevel_++;
    node.left()->accept(*this);
    node.right()->accept(*this);
    indentLevel_--;
}

void ASTDumper::visit(UnaryExpr& node) {
    indent();
    out_ << "UnaryExpr: '" << unaryOpToString(node.op())
         << "' [" << node.location().toString() << "]\n";
    indentLevel_++;
    node.operand()->accept(*this);
    indentLevel_--;
}

void ASTDumper::visit(LiteralExpr& node) {
    indent();
    out_ << "LiteralExpr: " << node.value()
         << " [" << node.location().toString() << "]\n";
}

void ASTDumper::visit(VarExpr& node) {
    indent();
    out_ << "VarExpr: " << node.name()
         << " [" << node.location().toString() << "]\n";
}

void ASTDumper::visit(AssignExpr& node) {
    indent();
    out_ << "AssignExpr: " << node.target() << " = [" << node.location().toString() << "]\n";
    indentLevel_++;
    node.value()->accept(*this);
    indentLevel_--;
}

void ASTDumper::visit(CallExpr& node) {
    indent();
    out_ << "CallExpr: " << node.callee()
         << " [" << node.location().toString() << "]\n";
    if (!node.args().empty()) {
        indentLevel_++;
        for (const auto& arg : node.args()) {
            arg->accept(*this);
        }
        indentLevel_--;
    }
}

} // namespace minic::ast
