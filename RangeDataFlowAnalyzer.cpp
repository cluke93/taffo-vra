#include "RangeDataFlowAnalyzer.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/CFG.h"

RangeDataflowAnalyzer::RangeDataflowAnalyzer(Function &Func) : F(Func)
{
    // Inizializza IN/OUT per ogni blocco
    for (BasicBlock &B : F)
    {
        IN[&B] = Range::BOTTOM;
        OUT[&B] = Range::BOTTOM;
    }
    // tipicamente l'entry parte da TOP
    IN[&F.getEntryBlock()] = Range::TOP;
}

void RangeDataflowAnalyzer::runWithSCEV(Function &F,
                                        LoopInfo &LI,
                                        ScalarEvolution &SE)
{
    // 1) Inietta via SCEV i PHI di induzione/accumulatore
    for (Loop *L : LI)
    {
        if (auto *CT = dyn_cast_or_null<SCEVConstant>(
                SE.getConstantMaxBackedgeTakenCount(L)))
        {
            int64_t trip = CT->getValue()->getSExtValue() + 1;
            BasicBlock *H = L->getHeader();
            for (Instruction &I : *H)
            {
                if (auto *PH = dyn_cast<PHINode>(&I))
                {
                    if (auto *AR = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(PH)))
                    {
                        if (auto *SC = dyn_cast<SCEVConstant>(AR->getStart()))
                        {
                            if (auto *ST = dyn_cast<SCEVConstant>(AR->getStepRecurrence(SE)))
                            {
                                float start = SC->getValue()->getSExtValue();
                                float step = ST->getValue()->getSExtValue();
                                float lo = start;
                                float hi = start + step * (trip - 1);
                                setValueRange(PH, Range(lo, hi));
                            }
                        }
                    }
                }
            }
        }
    }

    // 2) Prepara la worklist: tutte le istruzioni non‐costanti e non‐PHI
    SmallVector<Instruction *, 128> worklist;
    for (BasicBlock &B : F)
    {
        for (Instruction &I : B)
        {
            if (!isa<ConstantInt>(&I) &&
                !isa<ConstantFP>(&I) &&
                !isa<PHINode>(&I))
            {
                worklist.push_back(&I);
                // inizializziamo il valore a bottom
                valueRange[&I] = Range::BOTTOM;
            }
        }
    }

    // 3) Iterazione a punto fisso
    while (!worklist.empty())
    {
        Instruction *I = worklist.pop_back_val();

        Range oldR = getValueRange(I);
        Range newR = oldR;

        // calcolo newR solo per operazioni di interesse
        if (I->isBinaryOp())
        {
            Range a = getValueRange(I->getOperand(0));
            Range b = getValueRange(I->getOperand(1));
            switch (I->getOpcode())
            {
            case Instruction::Add:
            case Instruction::FAdd:
                newR = RangeHandler::Add(a, b);
                break;
            case Instruction::Sub:
            case Instruction::FSub:
                newR = RangeHandler::Sub(a, b);
                break;
                // case Instruction::Mul:
                // case Instruction::FMul:
                //   newR = RangeHandler::mul(a, b);
                //   break;
                // case Instruction::SDiv:
                // case Instruction::FDiv:
                //   newR = RangeHandler::div(a, b);
                break;
            default:
                break;
            }
        }
        // altrimenti newR = oldR

        // se cambia, aggiornalo e rischedula gli utenti
        if (newR.min != oldR.min || newR.max != oldR.max)
        {
            setValueRange(I, newR);
            for (User *U : I->users())
            {
                if (auto *UI = dyn_cast<Instruction>(U))
                {
                    // salta i PHI, che sono stati già iniettati
                    if (!isa<PHINode>(UI))
                        worklist.push_back(UI);
                }
            }
        }
    }
}

Range RangeDataflowAnalyzer::getValueRange(Value *V) const
{
    if (auto *CI = dyn_cast<ConstantInt>(V))
    {
        float v = static_cast<float>(CI->getSExtValue());
        return {v, v};
    }
    if (auto *CF = dyn_cast<ConstantFP>(V))
    {
        float v = CF->getValueAPF().convertToFloat();
        return {v, v};
    }
    auto it = valueRange.find(V);
    return it != valueRange.end() ? it->second : Range::TOP;
}

Range RangeDataflowAnalyzer::getRangeOf(Value *V, DenseMap<Value *, Range> &localRange) const
{
    if (auto *CI = dyn_cast<ConstantInt>(V))
    {
        float v = static_cast<float>(CI->getSExtValue());
        return {v, v};
    }
    if (auto *CF = dyn_cast<ConstantFP>(V))
    {
        float v = CF->getValueAPF().convertToFloat();
        return {v, v};
    }
    // altrimenti cerco nella map locale
    auto it = localRange.find(V);
    if (it != localRange.end())
        return it->second;
    // fallback conservativo
    return Range::TOP;
}

Range RangeDataflowAnalyzer::getOutRange(BasicBlock *B) const
{
    return OUT.lookup(B);
}

void RangeDataflowAnalyzer::populateScope(Scope *scope) const
{
    // Prima gli argomenti della funzione
    for (auto &Arg : F.args())
    {
        if (!Arg.hasName())
            continue;
        Range r = getValueRange(&Arg);
        scope->addVar(Arg.getName().str(), r);
    }

    // Poi tutte le istruzioni non-void
    for (auto &entry : valueRange)
    {
        llvm::Value *V = entry.first;
        const Range &r = entry.second;
        if (auto *I = llvm::dyn_cast<llvm::Instruction>(V))
        {
            if (I->getType()->isVoidTy())
                continue;
            if (!I->hasName())
                continue;
            scope->addVar(I->getName().str(), r);
        }
    }
}

void RangeDataflowAnalyzer::setValueRange(Value *V, const Range &R)
{
    valueRange[V] = R;
}