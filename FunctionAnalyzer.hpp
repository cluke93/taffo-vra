#ifndef FUNCTION_H
#define FUNCTION_H

#include "BlockClass.hpp"
#include "llvm/IR/Dominators.h"

#include "queue"
#include <algorithm>

namespace llvm {
    class VRAPass;
}

class FunctionAnalyzer {

public:

    std::string getName() {
        return el->getName().str();
    }

    VRAPass* getPass() {
        return vra_pass;
    } 

    Scope* getScope() {
        return scope.get();
    }

    LoopInfo* getLoopInfo() {
        return loopInfo;
    }

    DominatorTree& getDominatorTree() {
        return *DT;
    }

    std::pair<u_int64_t, u_int64_t> getLoopIterBounds(llvm::Loop* L);

    

    

    void initLoop(Block* header);

    /**
     * Loop latch: incrementa i loop latch visitati e analizzalo come fosse un blocco semplice.
     * Se tutti i latches sono stati visitati ricalcola lo scope del loop header, quindi accoda i loop exit
     */
    void processLoopLatch(Block* latch);

    void handleLoopExit(Block* header);

    void initStandardFork(Block* fork);

    void handleStandardMerge(Block* join);

    void processSimpleBlock(Block* block);

    void handleReturnBlock(Block* ret);



    void analyze();

    bool contains(BasicBlock* bb) const;


    /// Aggiunge un blocco già costruito a ownedBlocks e ne restituisce il puntatore grezzo
    Block* addBlock(std::unique_ptr<Block> block);

    /// Crea un nuovo Block in place, lo registra in ownedBlocks e ne restituisce il puntatore
    template<typename... Args>
    Block* emplaceBlock(Args&&... args);

    Block* getBlockByLLVMBasicBlock(llvm::BasicBlock* bb) const;

    /**
     * Add the block aggregation already created
     */
    void addBreadcrumb(BlockAggregation* agg);

    /**
     * Create a new block aggregation, append it and returns its poiter
     */
    BlockAggregation* emplaceBreadcrumb(AggregationType type, Block* ref);

    /**
     * Rimuove l'ultimo elemento della breadcrumb
     */
    void popBreadcrumb();

    /**
     * Put a basic block of the workload for future analysis
     */
    void enqueueBlock(BasicBlock* B);

    /**
     * Riconosci la keyword "break" del loop (non è break se il successore rimane nello stesso loop o va in uno innestato)
     */
    bool isNotBreakLoopKeyword(BasicBlock *BB, BasicBlock *Succ);

    Block* getLoopHeaderFromBreadcrumb() const;

    Block* getForkFromBreadcrumb() const;

    void setAnalysisBound();

    FunctionAnalyzer (Function* el, llvm::VRAPass* vra_pass);

private:

    /**
     * Pointer to llvm function object
     */
    Function* el;

    /**
     * Reference of pass
     */
    VRAPass* vra_pass = nullptr;

    /**
     * Scope of the function (args and returned elements)
     */
    std::unique_ptr<Scope> scope;

    /**
     * Vector where I can store the real block objects
     */
    std::vector<std::unique_ptr<Block>> ownedBlocks;

    /**
     * Ordered list of macro-blocks. Useful to reconstruct the struct of them
     */
    std::vector<BlockAggregation*> breadcrumb;

    /**
     * Queue of block to analyze
     */
    std::queue<BasicBlock*> workload;

    /**
     * Loop info of the function
     */
    LoopInfo* loopInfo;

    /**
     * LLVM function analysis manager
     */
    FunctionAnalysisManager& FAM;
    
    /**
     * Scalar evolution analysis of the function
     */
    ScalarEvolution& SE;

    /**
     * Container for dominator tree analysis
     */
    DominatorTree* DT;

    /**
     * Reference of instruction analyzer
     */
    std::shared_ptr<InstructionAnalyzer> IA;

};

#endif