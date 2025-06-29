
#pragma once

#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "RangeHandler.hpp"
#include "ScopeHandler.hpp"
#include <map>

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/LoopInfo.h"

using namespace llvm;

class RangeDataflowAnalyzer
{
public:
    RangeDataflowAnalyzer(Function &F);

    /// Esegue l’analisi dataflow, popola IN/OUT per ogni BasicBlock
    void run();

    void runWithSCEV(Function &F, LoopInfo &LI, ScalarEvolution &SE);

    /// Se vuoi accedere al range in uscita di un blocco
    Range getOutRange(BasicBlock *B) const;

    /// Helper per estrarre il Range di un Value (costante o già calcolato)
    Range getRangeOf(Value *V, DenseMap<Value *, Range> &localMap) const;

    /// Ritorna il range già calcolato per V, o TOP se non trovato
    Range getValueRange(Value *V) const;

    /// Popola lo scope con tutti i valori (argomenti e istruzioni) analizzati
    void populateScope(Scope *scope) const;

    void setValueRange(Value *V, const Range &R);

private:
    Function &F;
    /// IN[B] = range in ingresso al block B
    DenseMap<BasicBlock *, Range> IN;
    /// OUT[B] = range in uscita da B
    DenseMap<BasicBlock *, Range> OUT;

    /// Join di tutti i predecessori di B: ⊔ OUT[P]
    Range joinPredecessors(BasicBlock *B) const;

    /// Transfer function di un blocco: calcola OUT[B] = f_B(IN[B])
    Range transferBlock(BasicBlock *B);

    /// Mappa da qualunque Value* (Instruction o Argument) al suo OUT range
    llvm::DenseMap<llvm::Value *, Range> valueRange;

    // per ogni loop, quanti iterazioni ci aspettiamo
    llvm::DenseMap<llvm::Loop *, int64_t> loopTripCount;
    // per ogni (istruzione, loop) quante volte è già stata schedulata
    llvm::DenseMap<std::pair<llvm::Instruction *, llvm::Loop *>, int> iterationCount;
};