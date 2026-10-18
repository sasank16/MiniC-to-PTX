#include "minic/semantic/SemanticAnalyzer.hpp"

namespace minic::semantic {

bool SemanticAnalyzer::analyze(ast::Program& program) {
    // Pass 1: Collect and declare all function signatures in the global scope
    for (const auto& fn : program.functions()) {
        std::vector<Type> paramTypes;
        for (const auto& p : fn->params()) {
            Type pType = Type::fromAST(p->type());
            if (pType.isVoid()) {
                diag_.error(p->location(), "Parameter '" + p->name() + "' cannot have type 'void'");
            }
            paramTypes.push_back(pType);
        }

        Type retType = Type::fromAST(fn->returnType());
        Symbol fnSym = Symbol::makeFunction(fn->name(), retType, paramTypes, fn->isKernel(), fn->location());

        if (!symTab_.declare(fnSym)) {
            diag_.error(fn->location(), "Redefinition of function '" + fn->name() + "'");
        }
    }

    // Pass 2: Analyze function bodies
    visit(program);

    return !diag_.hasErrors();
}

void SemanticAnalyzer::visit(ast::Program& node) {
    for (const auto& fn : node.functions()) {
        fn->accept(*this);
    }
}

void SemanticAnalyzer::visit(ast::FunctionDecl& node) {
    currentFunction_ = &node;
    symTab_.enterScope(); // Scope for parameters and function body

    // Declare parameters in the function scope
    for (const auto& p : node.params()) {
        p->accept(*this);
    }

    if (node.body()) {
        isFunctionBody_ = true;
        node.body()->accept(*this);
        isFunctionBody_ = false;
    }

    symTab_.exitScope();
    currentFunction_ = nullptr;
}

void SemanticAnalyzer::visit(ast::ParamDecl& node) {
    Type pType = Type::fromAST(node.type());
    Symbol sym = Symbol::makeParameter(node.name(), pType, node.location());
    if (!symTab_.declare(sym)) {
        diag_.error(node.location(), "Duplicate parameter name '" + node.name() + "'");
    }
}

void SemanticAnalyzer::visit(ast::CompoundStmt& node) {
    bool isTopBody = isFunctionBody_;
    isFunctionBody_ = false;

    if (!isTopBody) {
        symTab_.enterScope();
    }

    for (const auto& stmt : node.statements()) {
        stmt->accept(*this);
    }

    if (!isTopBody) {
        symTab_.exitScope();
    }
}

void SemanticAnalyzer::visit(ast::VarDeclStmt& node) {
    Type varType = Type::fromAST(node.type());
    if (varType.isVoid()) {
        diag_.error(node.location(), "Variable '" + node.name() + "' cannot have type 'void'");
        return;
    }

    if (node.init()) {
        Type initType = checkExpression(node.init());
        if (!initType.isError() && initType != varType) {
            diag_.error(node.init()->location(),
                        "Type mismatch in initializer for '" + node.name() + "': cannot assign '" +
                        initType.toString() + "' to '" + varType.toString() + "'");
        }
    }

    Symbol sym = Symbol::makeVariable(node.name(), varType, node.location());
    if (!symTab_.declare(sym)) {
        diag_.error(node.location(), "Redeclaration of variable '" + node.name() + "' in the same scope");
    }
}

void SemanticAnalyzer::visit(ast::ExprStmt& node) {
    if (node.expr()) {
        checkExpression(node.expr());
    }
}

void SemanticAnalyzer::visit(ast::IfStmt& node) {
    Type condType = checkExpression(node.condition());
    if (!condType.isError() && !condType.isInt()) {
        diag_.error(node.condition()->location(), "If condition must evaluate to integer type");
    }

    if (node.thenBranch()) {
        node.thenBranch()->accept(*this);
    }
    if (node.elseBranch()) {
        node.elseBranch()->accept(*this);
    }
}

void SemanticAnalyzer::visit(ast::WhileStmt& node) {
    Type condType = checkExpression(node.condition());
    if (!condType.isError() && !condType.isInt()) {
        diag_.error(node.condition()->location(), "While condition must evaluate to integer type");
    }

    if (node.body()) {
        node.body()->accept(*this);
    }
}

void SemanticAnalyzer::visit(ast::ReturnStmt& node) {
    if (!currentFunction_) {
        diag_.error(node.location(), "'return' statement outside of any function");
        return;
    }

    Type expectedRetType = Type::fromAST(currentFunction_->returnType());

    if (node.value()) {
        if (expectedRetType.isVoid()) {
            diag_.error(node.location(), "Cannot return a value from void function '" + currentFunction_->name() + "'");
        } else {
            Type valType = checkExpression(node.value());
            if (!valType.isError() && valType != expectedRetType) {
                diag_.error(node.value()->location(),
                            "Return value type mismatch: expected '" + expectedRetType.toString() +
                            "', got '" + valType.toString() + "'");
            }
        }
    } else {
        if (!expectedRetType.isVoid()) {
            diag_.error(node.location(),
                        "Non-void function '" + currentFunction_->name() + "' must return a value of type 'int'");
        }
    }
}

Type SemanticAnalyzer::checkExpression(ast::Expr* expr) {
    if (!expr) return Type::getError();
    expr->accept(*this);
    return lastExprType_;
}

void SemanticAnalyzer::visit(ast::BinaryExpr& node) {
    Type leftType = checkExpression(node.left());
    Type rightType = checkExpression(node.right());

    if (leftType.isError() || rightType.isError()) {
        lastExprType_ = Type::getError();
        return;
    }

    if (!leftType.isInt() || !rightType.isInt()) {
        diag_.error(node.location(), "Operands to binary operator '" +
                    std::string(ast::binaryOpToString(node.op())) + "' must be integers");
        lastExprType_ = Type::getError();
        return;
    }

    // Static division-by-zero check
    if (node.op() == ast::BinaryOp::Div || node.op() == ast::BinaryOp::Rem) {
        auto* rightLit = dynamic_cast<ast::LiteralExpr*>(node.right());
        if (rightLit && rightLit->value() == 0) {
            diag_.error(node.right()->location(), "Division or modulo by zero is undefined");
            lastExprType_ = Type::getError();
            return;
        }
    }

    // In MiniC, comparisons and arithmetic all yield int (s32)
    lastExprType_ = Type::getInt();
}

void SemanticAnalyzer::visit(ast::UnaryExpr& node) {
    Type opType = checkExpression(node.operand());
    if (opType.isError()) {
        lastExprType_ = Type::getError();
        return;
    }

    if (!opType.isInt()) {
        diag_.error(node.location(), "Operand to unary operator '" +
                    std::string(ast::unaryOpToString(node.op())) + "' must be integer");
        lastExprType_ = Type::getError();
        return;
    }

    lastExprType_ = Type::getInt();
}

void SemanticAnalyzer::visit(ast::LiteralExpr& node) {
    (void)node;
    lastExprType_ = Type::getInt();
}

void SemanticAnalyzer::visit(ast::VarExpr& node) {
    const Symbol* sym = symTab_.lookup(node.name());
    if (!sym) {
        diag_.error(node.location(), "Use of undeclared variable '" + node.name() + "'");
        lastExprType_ = Type::getError();
        return;
    }

    if (sym->kind == SymbolKind::Function) {
        diag_.error(node.location(), "Cannot use function '" + node.name() + "' as a variable value");
        lastExprType_ = Type::getError();
        return;
    }

    lastExprType_ = sym->type;
}

void SemanticAnalyzer::visit(ast::AssignExpr& node) {
    const Symbol* sym = symTab_.lookup(node.target());
    if (!sym) {
        diag_.error(node.location(), "Assignment to undeclared variable '" + node.target() + "'");
        lastExprType_ = Type::getError();
        return;
    }

    if (sym->kind == SymbolKind::Function) {
        diag_.error(node.location(), "Cannot assign to function '" + node.target() + "'");
        lastExprType_ = Type::getError();
        return;
    }

    Type valType = checkExpression(node.value());
    if (valType.isError()) {
        lastExprType_ = Type::getError();
        return;
    }

    if (sym->type != valType) {
        diag_.error(node.value()->location(),
                    "Type mismatch in assignment to '" + node.target() + "': cannot assign '" +
                    valType.toString() + "' to '" + sym->type.toString() + "'");
        lastExprType_ = Type::getError();
        return;
    }

    lastExprType_ = sym->type;
}

void SemanticAnalyzer::visit(ast::CallExpr& node) {
    const Symbol* sym = symTab_.lookup(node.callee());
    if (!sym) {
        diag_.error(node.location(), "Call to undeclared function '" + node.callee() + "'");
        lastExprType_ = Type::getError();
        return;
    }

    if (sym->kind != SymbolKind::Function) {
        diag_.error(node.location(), "Cannot call non-function '" + node.callee() + "'");
        lastExprType_ = Type::getError();
        return;
    }

    if (node.args().size() != sym->paramTypes.size()) {
        diag_.error(node.location(),
                    "Function '" + node.callee() + "' expects " +
                    std::to_string(sym->paramTypes.size()) + " argument(s), but " +
                    std::to_string(node.args().size()) + " were provided");
        lastExprType_ = sym->type;
        return;
    }

    for (size_t i = 0; i < node.args().size(); ++i) {
        Type argType = checkExpression(node.args()[i].get());
        if (!argType.isError() && argType != sym->paramTypes[i]) {
            diag_.error(node.args()[i]->location(),
                        "Argument " + std::to_string(i + 1) + " of function '" + node.callee() +
                        "' expects type '" + sym->paramTypes[i].toString() + "', but got '" +
                        argType.toString() + "'");
        }
    }

    lastExprType_ = sym->type;
}

} // namespace minic::semantic
