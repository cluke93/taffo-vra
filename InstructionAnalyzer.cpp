#include "InstructionAnalyzer.hpp"
#include "BlockClass.hpp"

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

    std::optional<Range> newRange;

    for (unsigned i = 0; i < phi->getNumIncomingValues(); ++i) {
        analyzePhiNodeBranch(phi, i, newRange);
    }

    curBlock->getScope()->addVar(varName, newRange.value());
}

void InstructionAnalyzer::analyzePHINodesLoopHeader(Instruction* I) {

    curInstruction = I;
    
    auto *phi = dyn_cast<PHINode>(curInstruction);
    if (phi == nullptr) return;
    
    kind = InstructionType::PHI;
    std::string varName = I->getName().str();

    std::optional<Range> newRange;

    analyzePhiNodeBranch(phi, 0, newRange);

    curBlock->getScope()->addVar(varName, newRange.value());
}

void InstructionAnalyzer::analyzePhiNodeBranch(PHINode* phi, int i, std::optional<Range>& newRange) {

    Value* incoming = phi->getIncomingValue(i);
    std::string name = incoming->getName().str();
    BasicBlock* incomingBlock = phi->getIncomingBlock(i);

    // Caso 1: costante
    if (isa<ConstantFP>(incoming)) {
        float val = cast<ConstantFP>(incoming)->getValueAPF().convertToFloat();

        if (!newRange.has_value()) {
            newRange = Range(val, val);
        } else {
            newRange->tryRangeEnlarging(Range(val, val));
        }
        

    } else if (isa<ConstantInt>(incoming)) {
        float val = static_cast<float>(cast<ConstantInt>(incoming)->getSExtValue());

        if (!newRange.has_value()) {
            newRange = Range(val, val);
        } else {
            newRange->tryRangeEnlarging(Range(val, val));
        }

    // Caso 2: variabile regolare -> cerca nello scope del blocco
    } else if (Scope* phiScope = curBlock->getScope()) {
        Var* v = phiScope->lookup(name);
        
        if (!newRange.has_value()) {
            newRange = v->range;
        } else {
            newRange->tryRangeEnlarging(v->range);
        }
        
    }

}

void InstructionAnalyzer::handleBinaryOp() {

    Value* op1 = curInstruction->getOperand(0);
    Value* op2 = curInstruction->getOperand(1);

    Range range1, range2;

    if (auto* c1 = dyn_cast<ConstantInt>(op1)) {
        float val = static_cast<float>(c1->getSExtValue());
        range1 = {val, val};

        if (!seenConstants.insert(val).second) { // solo se non già vista
            // se c'è da fare qualcosa sulle costanti va fatta qui
        }

    } else if (auto* c1f = dyn_cast<ConstantFP>(op1)) {
        float val = static_cast<float>(c1f->getValueAPF().convertToFloat());
        range1 = {val, val};

        if (seenConstants.insert(val).second) {
            // se c'è da fare qualcosa sulle costanti va fatta qui
        }
    } else if (op1->hasName()) {
        if (Var* var = curBlock->getScope()->lookup(op1->getName().str())) {
            range1 = var->range;
        } else {
            return; // variabile non trovata
        }
    } else {
        return;
    }

    // --- Operando 2 ---
    if (auto* c2 = dyn_cast<ConstantInt>(op2)) {
        float val = static_cast<float>(c2->getSExtValue());
        range2 = {val, val};

        if (seenConstants.insert(val).second) {
            // se c'è da fare qualcosa sulle costanti va fatta qui
        }

    } else if (auto* c2f = dyn_cast<ConstantFP>(op2)) {
        float val = static_cast<float>(c2f->getValueAPF().convertToFloat());
        range2 = {val, val};

        if (seenConstants.insert(val).second) {
            // se c'è da fare qualcosa sulle costanti va fatta qui
        }
    } else if (op2->hasName()) {
        if (Var* var = curBlock->getScope()->lookup(op2->getName().str())) {
            range2 = var->range;
        } else {
            return;
        }
    } else {
        return;
    }

    // --- Calcolo del range del risultato ---
    Range resultRange;
    switch (curInstruction->getOpcode()) {
        case Instruction::Add:
            resultRange = RA.get()->Add(range1, range2, curMinIter, curMaxIter);
            
            break;
        case Instruction::Sub:
            resultRange = RA.get()->Sub(range1, range2, curMinIter, curMaxIter);
            
            break;
        case Instruction::Mul: {
            if (curMaxIter == 1) {
                resultRange = RA.get()->Mul(range1, range2);
            } else {
                resultRange = RA.get()->MulOnLoop(range1, range2, curMinIter, curMaxIter);
            }
            
            break;
        }

        case Instruction::SDiv:
            resultRange = RA.get()->Div(range1, range2);
            break;

        default:
            return;
    }

    // --- Salva risultato ---
    std::string resName = curInstruction->getName().str();
    curBlock->getScope()->addVar(resName, resultRange);

    // errs() << "  [VRA] " << resName << " = " << curInstruction->getOpcodeName()
    //        << "(" << op1->getName() << ", " << op2->getName() << ")  => ["
    //        << Utils::formatFloatSmart(resultRange.min) << ", " << Utils::formatFloatSmart(resultRange.max) << "]\n";

}

void InstructionAnalyzer::handleUnaryOp() {
    std::string resName = curInstruction->getName().str();
    Range resultRange;

    switch (curInstruction->getOpcode()) {
        case Instruction::FNeg:
        case Instruction::Sub:
            if (curInstruction->getNumOperands() == 1 || 
                (curInstruction->getOperand(0)->getName().empty() && isa<Constant>(curInstruction->getOperand(0)))) {

                Range range;

                if (auto* c1 = dyn_cast<ConstantInt>(curInstruction->getOperand(0))) {
                    float val = static_cast<float>(c1->getSExtValue());
                    range = {val, val};
            
                    if (!seenConstants.insert(val).second) { // solo se non già vista
                        // se c'è da fare qualcosa sulle costanti va fatta qui
                    }
            
                } else if (auto* c1f = dyn_cast<ConstantFP>(curInstruction->getOperand(0))) {
                    float val = static_cast<float>(c1f->getValueAPF().convertToFloat());
                    range = {val, val};
            
                    if (seenConstants.insert(val).second) {
                        // se c'è da fare qualcosa sulle costanti va fatta qui
                    }
                } else if (curInstruction->getOperand(0)->hasName()) {
                    if (Var* var = curBlock->getScope()->lookup(curInstruction->getOperand(0)->getName().str())) {
                        range = var->range;
                    } else {
                        return; // variabile non trovata
                    }
                } else {
                    return;
                }
                
                // Cambio di segno: -x → range [-b, -a]
                resultRange = Range(-range.max, -range.min);
                curBlock->getScope()->addVar(resName, resultRange);
            }
            break;

        case Instruction::Xor:
            // Bitwise NOT di un valore intero: inverti i bit.
            // Conservativamente, puoi dire che il range è sconosciuto ma limitato.
            // Ad esempio: [-MAX_INT, MAX_INT]
            // TODO: check per fare meglio
            resultRange = {-std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
            curBlock->getScope()->addVar(resName, resultRange);

        default:
            break;
    }

}

void InstructionAnalyzer::handleCmp() {
    std::string resName = curInstruction->getName().str();
    curBlock->getScope()->addVar(resName, Range(0.0f, 1.0f));
}

