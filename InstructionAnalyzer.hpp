#ifndef VRA_INSTRUCTION_ANALYZER_H
#define VRA_INSTRUCTION_ANALYZER_H

#include "llvm/IR/Instruction.h"

#include "Utils.hpp"
#include "RangeHandler.hpp"

using namespace llvm;

enum InstructionType {
    Binary, Unary, Call, PHI, Boolean
};

class Block;

class InstructionAnalyzer {

    public:

        

        /**
         * Analyze only phi nodes to allow a cleaner join block management
         */
        void analyzePHINodes(Instruction* I);

        /**
         * Analyze only the predecessor branch of phi nodes
         */
        void analyzePHINodesLoopHeader(Instruction* I);

        void analyzePhiNodeBranch(PHINode* phi, int i);

        /**
         * Analyze only binary and unary math expressions which modify ranges on the scope
         */
        void analyzeExpressionNodes(Instruction* I);


        void analyze(Instruction* I);

        void loadBlock(Block* b);

        void setCurrentIterBounds(std::pair<u_int64_t, u_int64_t> bounds) {
            curMinIter = bounds.first;
            curMaxIter = bounds.second;
        }

        /**
         * Function to free curBlock. Suggested to avoid to edit scopes different
         */
        void freeBlock() {
            curBlock = nullptr;
        }

        void handleBinaryOp();
        void handleUnaryOp();
        
        void handleCmp();

        InstructionAnalyzer(std::shared_ptr<RangeHandler> RA): RA(RA) {}

protected:

    std::optional<Range> getConstRange(Value* val);


    private:

    /**
     * Current instruction
     */
    Instruction* curInstruction = nullptr;

    /**
     * Current block
     */
    Block* curBlock = nullptr;

    /**
     * Class with useful static method to work with ranges
     */
    std::shared_ptr<RangeHandler> RA;

    /**
     * Kind of last analyzed instruction
     */
    InstructionType kind;

    /**
     * Pool of already seen constants
     */
    std::set<float> seenConstants;  

    /**
     * How many iterations has this instruction (> 1 is inside loop)
     */
    int curMaxIter = 1;

    /**
     * Number of minimum iteration of this instruction (= 0 is loop never executed)
     */
    int curMinIter = 1;

};


#endif