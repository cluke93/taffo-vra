#pragma once

#ifndef BLOCK_VISITOR_H
#define BLOCK_VISITOR_H

#include "BlockClass.hpp"

using namespace llvm;

class BlockVisitor {
public:
    virtual ~BlockVisitor() = default;

    virtual void visitLoopHeader(Block* header) = 0;
    
    virtual void visitLoopLatch(Block* latch) = 0;

    virtual void visitLoopExit(Block* exit) = 0;
    
    virtual void visitStandardFork(Block* fork) = 0;

    virtual void visitInterLoopFork(Block* fork) = 0;

    virtual void visitStandardMerge(Block* join) = 0;
    
    virtual void visitSimple(Block* simple) = 0;

    virtual void visitReturnBlock(Block* ret) = 0;
};

#endif