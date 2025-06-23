#ifndef RANG_PROP_VISIT_H
#define RANG_PROP_VISIT_H

#include "BlockVisitor.hpp"
#include "FunctionAnalyzer.hpp"

class RangePropagationVisitor : public BlockVisitor {
public:
    // reference to the global analyzer state
    FunctionAnalyzer* owner;

    RangePropagationVisitor(FunctionAnalyzer* fn) : owner(fn) {}

    void visitLoopHeader(Block* header) override {
        // initialize or merge header scope
        owner->emplaceBreadcrumb(AggregationType::Loop, header);
        owner->initLoop(header);
    }

    void visitLoopLatch(Block* latch) override {
        // update ranges on latch
        owner->processLoopLatch(latch);
    }

    void visitStandardFork(Block* fork) override {
        // merge fork-init comparisons
        owner->initStandardFork(fork);
    }

    void visitStandardMerge(Block* join) override {
        // merge branch scopes
        owner->handleStandardMerge(join);
    }

    void visitSimple(Block* simple) override {
        // default successor enqueue
        owner->processSimpleBlock(simple);
    }

    void visitLoopExit(Block* exit) override {
        // default successor enqueue
        owner->processSimpleBlock(exit);
    }

    void visitInterLoopFork(Block* fork) override {
        // default successor enqueue
        owner->processSimpleBlock(fork);
    }

    void visitReturnBlock(Block* ret) override {
        owner->handleReturnBlock(ret);
    }
};

#endif