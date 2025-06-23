#ifndef BLOCK_H
#define BLOCK_H

#include "BlockVisitor.hpp"
#include "ScopeHandler.hpp"
#include "InstructionAnalyzer.hpp"

#include <string>

using namespace llvm;

enum BlockTypology {
    StandardFork,       // fork with classic join block later without outgoing from current loop
    InterLoopFork,      // fork within loop where join point is outside of its loop (break keyword)
    StandardMerge,      // jojn point derived from StardardFork
    LoopHeader,         // first block of the loop with phi nodes
    LoopLatch,          // block which connect loop header, used to understand when all blocks inside loop are analyzed
    LoopExit,           // ExitBlock of the loop, can update unmerged inner loop fork
    ReturnBlock,        // block with Ret instruction
    SimpleBlock         // block without particularities, just follow it
};

enum LoopRelation {
    SameLoop,
    NestedLoop,
    OutsideLoop
};

enum AggregationType {
    Fork, Loop
};

struct BlockAggregation {
    AggregationType type;
    Block* ref;

    BlockAggregation(AggregationType type, Block* ref) : type(type), ref(ref) {}
};

class Block {

public:

    std::string getName() const;

    /**
     * Full logic to recognize the type of the block
     */
    void recognize();

    /**
     * Let's analyze the block, discovering parents, children and computing the scope
     */
    bool accept(BlockVisitor& visitor);

    /**
     * Return loop reference of this block if any
     */
    llvm::Loop* getLoop();

     /**
     * Create, initialize and add scope of the block
     */
    Scope* emplaceScope(Scope* parent, bool isLoopScope);

    /**
     * Set the initial number of latches to analyze before terminate loop analysis
     */
    void setNumLoopLatches(llvm::Loop* L);

    /**
     * Reduce the number of remaining latches by 1
     */
    void decrRemainingLatches();

    /**
     * Check if all latches were been analyzed
     */
    bool isLoopWholeAnalyzed();

    /**
     * Set the initial number of branches to analyze before handle merge
     */
    void setNumBranches(short num_branches);

    /**
     * Reduce the number of remaining branches by 1
     */
    void decrRemainingBranches();

    /**
     * Check if all branches were been analyzed
     */
    bool isForkWholeAnalyzed();

    void rescaleLoopHeaderScope(Scope* latchScope);

    FunctionAnalyzer* getOwner();

    BlockTypology getType();

    void setType(BlockTypology t);

    Scope* getScope();

    BasicBlock* getLLVMBasicBlock();

    void setIterBounds(std::pair<u_int64_t, u_int64_t> IB);

    std::pair<u_int64_t, u_int64_t> getIterBounds();

    Block(BasicBlock* el, FunctionAnalyzer* owner);

protected:

    /**
     * Return true if this is a loop header block
     */
    bool isLoopHeader() {
        return type == BlockTypology::LoopHeader;
    }

    /**
     * Return true if this is a loop latch block
     */
    bool isLoopLatch() {
        return type == BlockTypology::LoopLatch;
    }

    /**
     * Return true if this is a loop exit block
     */
    bool isLoopExit() {
        return type == BlockTypology::LoopExit;
    }

    /**
     * Return true if this is a simple block
     */
    bool isSimpleBlock() {
        return type == BlockTypology::SimpleBlock;
    }

    /**
     * Return true if this is a standard fork
     */
    bool isStandardFork() {
        return type == BlockTypology::StandardFork;
    }

    /**
     * Return true if this is a standard merge
     */
    bool isStandardMerge() {
        return type == BlockTypology::StandardMerge;
    }

private:

    /**
     * Pointer to referenced llvm basic block
     */
    BasicBlock* el;

    /**
     * Type of the block
     */
    BlockTypology type;

    /**
     * If not null it belong to pointed loop
     */
    llvm::Loop* ownedByLoop = nullptr;

    /**
     * Pointer to holder function
     */
    FunctionAnalyzer* ownedByFunc;

    /**
     * Info about the iterative behavior of this block
     */
    std::pair<u_int64_t, u_int64_t> bounds = {0 , 1};

    /**
     * Num of loop latches to know when all loop path have been analyzed
     */
    short num_latches = 0;

    /**
     * Num of fork branches to know when all loop path have been analyzed
     */
    short num_branches = 0;

    /**
     * Scope of the block
     */
    std::unique_ptr<Scope> scope;

    /**
     * Pointer to holder function
     */
    FunctionAnalyzer* owner;
};


#endif