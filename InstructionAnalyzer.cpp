#include "InstructionAnalyzer.hpp"
#include "BlockClass.hpp"

static int ConstNameCounter = 0;

static std::string makeConstName() {
    return "const" + std::to_string(++ConstNameCounter);
}



void InstructionAnalyzer::loadBlock(Block* b) {
    curBlock = b;
}

void InstructionAnalyzer::analyzeExpressionNodes(Instruction* I) {
    curInstruction = I;

    if (curInstruction->isBinaryOp()) {
        kind = InstructionType::Binary;
        handleBinaryOp();
    }

    else if (curInstruction->isUnaryOp()) {
        kind = InstructionType::Unary;
        handleUnaryOp();
    }
    
    else if (dyn_cast<CmpInst>(curInstruction)) {
        kind = InstructionType::Boolean;
        handleCmp();
    }
}

void InstructionAnalyzer::analyzePHINodes(Instruction* I) {
    curInstruction = I;
    
    auto *phi = dyn_cast<PHINode>(curInstruction);
    if (phi == nullptr) return;
    
    kind = InstructionType::PHI;
    std::string varName = I->getName().str();
    Scope* phiScope = curBlock->getScope();

    std::vector<Operand*> dependencies;

    for (unsigned i = 0; i < phi->getNumIncomingValues(); ++i) {
        
        Value* incoming = phi->getIncomingValue(i);
       

        if (auto op = getConstRange(incoming)) {

            std::string name = makeConstName();
            dependencies.push_back(new Operand(name, *op, VarType::Constant));

        } else if (Operand* existing = phiScope->lookup(incoming->getName().str())) {
            existing->tryResolution();
            dependencies.push_back(existing);
        } else {
            //TODO: gestione situazione
            errs() << "analyzePhiNodeBranch sconosciuto\n";
            return;
        }

    }

    std::function<Range(const std::vector<Range>&)> callFn = [](const std::vector<Range>& args) {
        if (args.empty()) {
            // nessun argomento: potresti restituire un range “infinito” o uno default
            return Range(NEG_INF, POS_INF);
        }
        // inizio dal primo
        Range acc = args[0];
        // mergio a cascata tutti gli altri
        for (size_t i = 1; i < args.size(); ++i) {
            acc = RangeHandler::Merge(acc, args[i]);
        }
        return acc;
    };

    // Now it's time to create the result operand and add it to the scope of the block
    auto resultOperand = std::make_unique<Operand>(curInstruction->getName().str(), dependencies, callFn, VarType::Local);
    resultOperand->tryResolution();

    curBlock->getScope()->addOperand(std::move(resultOperand));
}

void InstructionAnalyzer::analyzePHINodesLoopHeader(Instruction* I) {

    curInstruction = I;
    
    auto *phi = dyn_cast<PHINode>(curInstruction);
    if (phi == nullptr) return;
    
    kind = InstructionType::PHI;
    std::string name = "HEAD_" + I->getName().str();
    Value* incoming = phi->getIncomingValue(0);
    Scope* phiScope = curBlock->getScope();

    std::unique_ptr<Operand> resultOp;

    if (auto op = getConstRange(incoming)) {

        resultOp = std::make_unique<Operand>(name, *op, VarType::Constant);

    } else if (Operand* existing = phiScope->lookup(incoming->getName().str())) {
        resultOp = existing->clone();
    } else {
        //TODO: gestione situazione
        errs() << "analyzePhiNodeBranch sconosciuto\n";
        return;
    }

    resultOp->tryResolution();
    curBlock->getScope()->addOperand(std::move(resultOp));
}

void InstructionAnalyzer::handleBinaryOp() {

    Value* op1 = curInstruction->getOperand(0);
    Value* op2 = curInstruction->getOperand(1);

    std::vector<Operand*> dependencies;

    if (auto opt1ConstRange = getConstRange(op1)) {
        
        std::string name1 = makeConstName();
        dependencies.push_back(new Operand(name1, *opt1ConstRange, VarType::Constant));

    } else if (op1->hasName()) {
        // è una variabile
        auto* left_op = curBlock->getScope()->lookup(op1->getName().str());
        left_op->tryResolution();
        dependencies.push_back(left_op);
    }

    if (auto opt2ConstRange = getConstRange(op2)) {
        std::string name2 = makeConstName();
        dependencies.push_back(new Operand(name2, *opt2ConstRange, VarType::Constant));
    } else if (op2->hasName()) {
        auto* right_op = curBlock->getScope()->lookup(op2->getName().str());
        right_op->tryResolution();
        dependencies.push_back(right_op);
    }


    std::function<Range(const std::vector<Range>&)> callFn;
    switch (curInstruction->getOpcode()) {
        case Instruction::Add: {
            callFn = [this](const std::vector<Range>& args) {
                    return RA.get()->Add(args[0], args[1], curMinIter, curMaxIter);
                };
            break;
        }
        case Instruction::Sub: {
            callFn = [this](const std::vector<Range>& args) {
                    return RA.get()->Sub(args[0], args[1], curMinIter, curMaxIter);
                };
            break;
        }
        case Instruction::Mul: {
            callFn = [this](const std::vector<Range>& args) {
                    return RA.get()->MulOnLoop(args[0], args[1], curMinIter, curMaxIter);
                };
            break;
        }
        case Instruction::SDiv: {
            // callFn = [this](const std::vector<Range>& args) {
            //         return RA.get()->Div(args[0], args[1], curMinIter, curMaxIter);
            //     };

            break;
        }
        default:
            callFn = nullptr;
            //TODO: op not handled yet
            return;
    }

    // Now it's time to create the result operand and add it to the scope of the block
    auto resultOperand = std::make_unique<Operand>(curInstruction->getName().str(), dependencies, callFn, VarType::Local);
    resultOperand->tryResolution();

    curBlock->getScope()->addOperand(std::move(resultOperand));
}

void InstructionAnalyzer::handleUnaryOp() {
    
}

void InstructionAnalyzer::handleCmp() {
    std::string resName = curInstruction->getName().str();
    curBlock->getScope()->addOperand(std::make_unique<Operand>(resName, Range(0.0f, 1.0f), VarType::Constant));
}


std::optional<Range> InstructionAnalyzer::getConstRange(Value* val) {

    if (auto* k_val = dyn_cast<ConstantInt>(val)) {
        float v = static_cast<float>(k_val->getSExtValue());
        return Range(v, v);
    } else if (auto* k_val = dyn_cast<ConstantFP>(val)) {
        float v = static_cast<float>(k_val->getValueAPF().convertToFloat());
        return Range(v, v);
    }

    return std::nullopt;
}
