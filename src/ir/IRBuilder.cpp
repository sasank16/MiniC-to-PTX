#include "minic/ir/IRBuilder.hpp"

namespace minic::ir {

void IRBuilder::connectEdge(BasicBlock* from, BasicBlock* to) {
    if (from && to) {
        from->addSuccessor(to);
        to->addPredecessor(from);
    }
}

std::unique_ptr<Module> IRBuilder::build(ast::Program& program) {
    visit(program);
    return std::move(module_);
}

void IRBuilder::visit(ast::Program& node) {
    for (const auto& fn : node.functions()) {
        fn->accept(*this);
    }
}

void IRBuilder::visit(ast::FunctionDecl& node) {
    std::vector<std::shared_ptr<Parameter>> params;
    for (const auto& p : node.params()) {
        params.push_back(std::make_shared<Parameter>(
            p->name(), semantic::Type::fromAST(p->type())));
    }

    currentFunction_ = module_->createFunction(
        node.name(), semantic::Type::fromAST(node.returnType()),
        std::move(params), node.isKernel());

    // Create entry basic block
    currentBlock_ = currentFunction_->createBasicBlock("entry");

    // Spill incoming parameters to local alloca slots so they can be read and mutated like variables
    for (const auto& param : currentFunction_->params()) {
        currentBlock_->appendInstruction(
            std::make_unique<AllocaInst>(param->name(), param->type()));
        currentBlock_->appendInstruction(
            std::make_unique<StoreInst>(param, param->name()));
    }

    if (node.body()) {
        node.body()->accept(*this);
    }

    // If the last block didn't have an explicit return, synthesize one
    if (currentBlock_ && !currentBlock_->isTerminated()) {
        if (currentFunction_->returnType().isVoid()) {
            currentBlock_->appendInstruction(std::make_unique<ReturnInst>(nullptr));
        } else {
            currentBlock_->appendInstruction(
                std::make_unique<ReturnInst>(std::make_shared<ConstantInt>(0)));
        }
    }

    currentFunction_ = nullptr;
    currentBlock_ = nullptr;
}

void IRBuilder::visit(ast::ParamDecl& node) {
    (void)node;
}

void IRBuilder::visit(ast::CompoundStmt& node) {
    for (const auto& stmt : node.statements()) {
        if (currentBlock_ && currentBlock_->isTerminated()) {
            // Subsequent instructions in this block are unreachable
            break;
        }
        stmt->accept(*this);
    }
}

void IRBuilder::visit(ast::VarDeclStmt& node) {
    if (!currentBlock_ || currentBlock_->isTerminated()) return;

    // Emit alloca
    currentBlock_->appendInstruction(std::make_unique<AllocaInst>(
        node.name(), semantic::Type::fromAST(node.type())));

    // If initialized, store value
    if (node.init()) {
        auto initVal = lowerExpression(node.init());
        if (initVal) {
            currentBlock_->appendInstruction(
                std::make_unique<StoreInst>(initVal, node.name()));
        }
    }
}

void IRBuilder::visit(ast::ExprStmt& node) {
    if (!currentBlock_ || currentBlock_->isTerminated()) return;
    if (node.expr()) {
        lowerExpression(node.expr());
    }
}

std::shared_ptr<VirtualRegister> IRBuilder::ensurePredicate(std::shared_ptr<Value> val) {
    auto vreg = std::dynamic_pointer_cast<VirtualRegister>(val);
    if (vreg && vreg->isPredicate()) {
        return vreg;
    }

    // Compare with 0: %p = cmp_ne val, 0
    auto pred = currentFunction_->createRegister(semantic::Type::getInt(), /*isPredicate=*/true);
    currentBlock_->appendInstruction(std::make_unique<CompareInst>(
        Opcode::CmpNe, pred, val, std::make_shared<ConstantInt>(0)));
    return pred;
}

void IRBuilder::visit(ast::IfStmt& node) {
    if (!currentBlock_ || currentBlock_->isTerminated()) return;

    auto condVal = lowerExpression(node.condition());
    auto predReg = ensurePredicate(condVal);

    BasicBlock* thenBlock = currentFunction_->createBasicBlock("if.then");
    BasicBlock* elseBlock = currentFunction_->createBasicBlock("if.else");
    BasicBlock* mergeBlock = currentFunction_->createBasicBlock("if.end");

    // Branch condition in current block
    currentBlock_->appendInstruction(
        std::make_unique<BranchCondInst>(predReg, thenBlock, elseBlock));
    connectEdge(currentBlock_, thenBlock);
    connectEdge(currentBlock_, elseBlock);

    // Emit then block
    currentBlock_ = thenBlock;
    if (node.thenBranch()) {
        node.thenBranch()->accept(*this);
    }
    if (!currentBlock_->isTerminated()) {
        currentBlock_->appendInstruction(std::make_unique<BranchInst>(mergeBlock));
        connectEdge(currentBlock_, mergeBlock);
    }

    // Emit else block
    currentBlock_ = elseBlock;
    if (node.elseBranch()) {
        node.elseBranch()->accept(*this);
    }
    if (!currentBlock_->isTerminated()) {
        currentBlock_->appendInstruction(std::make_unique<BranchInst>(mergeBlock));
        connectEdge(currentBlock_, mergeBlock);
    }

    currentBlock_ = mergeBlock;
}

void IRBuilder::visit(ast::WhileStmt& node) {
    if (!currentBlock_ || currentBlock_->isTerminated()) return;

    BasicBlock* condBlock = currentFunction_->createBasicBlock("while.cond");
    BasicBlock* bodyBlock = currentFunction_->createBasicBlock("while.body");
    BasicBlock* exitBlock = currentFunction_->createBasicBlock("while.end");

    // Jump from current block to condition block
    currentBlock_->appendInstruction(std::make_unique<BranchInst>(condBlock));
    connectEdge(currentBlock_, condBlock);

    // Emit condition block
    currentBlock_ = condBlock;
    auto condVal = lowerExpression(node.condition());
    auto predReg = ensurePredicate(condVal);
    currentBlock_->appendInstruction(
        std::make_unique<BranchCondInst>(predReg, bodyBlock, exitBlock));
    connectEdge(condBlock, bodyBlock);
    connectEdge(condBlock, exitBlock);

    // Emit body block
    currentBlock_ = bodyBlock;
    if (node.body()) {
        node.body()->accept(*this);
    }
    if (!currentBlock_->isTerminated()) {
        currentBlock_->appendInstruction(std::make_unique<BranchInst>(condBlock));
        connectEdge(currentBlock_, condBlock);
    }

    currentBlock_ = exitBlock;
}

void IRBuilder::visit(ast::ReturnStmt& node) {
    if (!currentBlock_ || currentBlock_->isTerminated()) return;

    if (node.value()) {
        auto retVal = lowerExpression(node.value());
        currentBlock_->appendInstruction(std::make_unique<ReturnInst>(retVal));
    } else {
        currentBlock_->appendInstruction(std::make_unique<ReturnInst>(nullptr));
    }
}

std::shared_ptr<Value> IRBuilder::lowerExpression(ast::Expr* expr) {
    if (!expr) return nullptr;
    expr->accept(*this);
    return lastValue_;
}

void IRBuilder::visit(ast::LiteralExpr& node) {
    lastValue_ = std::make_shared<ConstantInt>(node.value());
}

void IRBuilder::visit(ast::VarExpr& node) {
    auto dest = currentFunction_->createRegister(semantic::Type::getInt());
    currentBlock_->appendInstruction(std::make_unique<LoadInst>(dest, node.name()));
    lastValue_ = dest;
}

void IRBuilder::visit(ast::AssignExpr& node) {
    auto val = lowerExpression(node.value());
    currentBlock_->appendInstruction(std::make_unique<StoreInst>(val, node.target()));
    lastValue_ = val;
}

void IRBuilder::visit(ast::BinaryExpr& node) {
    auto lhs = lowerExpression(node.left());
    auto rhs = lowerExpression(node.right());

    Opcode op;
    bool isComparison = false;

    switch (node.op()) {
        case ast::BinaryOp::Add: op = Opcode::Add; break;
        case ast::BinaryOp::Sub: op = Opcode::Sub; break;
        case ast::BinaryOp::Mul: op = Opcode::Mul; break;
        case ast::BinaryOp::Div: op = Opcode::SDiv; break;
        case ast::BinaryOp::Rem: op = Opcode::SRem; break;
        case ast::BinaryOp::Eq:  op = Opcode::CmpEq; isComparison = true; break;
        case ast::BinaryOp::Ne:  op = Opcode::CmpNe; isComparison = true; break;
        case ast::BinaryOp::Lt:  op = Opcode::CmpLt; isComparison = true; break;
        case ast::BinaryOp::Le:  op = Opcode::CmpLe; isComparison = true; break;
        case ast::BinaryOp::Gt:  op = Opcode::CmpGt; isComparison = true; break;
        case ast::BinaryOp::Ge:  op = Opcode::CmpGe; isComparison = true; break;
        case ast::BinaryOp::And:
        case ast::BinaryOp::Or:
            // For logical And/Or, treat as comparison or bitwise
            op = (node.op() == ast::BinaryOp::And) ? Opcode::Mul : Opcode::Add;
            break;
    }

    if (isComparison) {
        auto dest = currentFunction_->createRegister(semantic::Type::getInt(), /*isPredicate=*/true);
        currentBlock_->appendInstruction(std::make_unique<CompareInst>(op, dest, lhs, rhs));
        lastValue_ = dest;
    } else {
        auto dest = currentFunction_->createRegister(semantic::Type::getInt(), /*isPredicate=*/false);
        currentBlock_->appendInstruction(std::make_unique<BinaryInst>(op, dest, lhs, rhs));
        lastValue_ = dest;
    }
}

void IRBuilder::visit(ast::UnaryExpr& node) {
    auto operand = lowerExpression(node.operand());

    if (node.op() == ast::UnaryOp::Plus) {
        lastValue_ = operand;
        return;
    }

    Opcode op = (node.op() == ast::UnaryOp::Neg) ? Opcode::Neg : Opcode::Not;
    auto dest = currentFunction_->createRegister(semantic::Type::getInt());
    currentBlock_->appendInstruction(std::make_unique<UnaryInst>(op, dest, operand));
    lastValue_ = dest;
}

void IRBuilder::visit(ast::CallExpr& node) {
    std::vector<std::shared_ptr<Value>> args;
    for (const auto& arg : node.args()) {
        args.push_back(lowerExpression(arg.get()));
    }

    auto dest = currentFunction_->createRegister(semantic::Type::getInt());
    currentBlock_->appendInstruction(
        std::make_unique<CallInst>(dest, node.callee(), std::move(args)));
    lastValue_ = dest;
}

} // namespace minic::ir
