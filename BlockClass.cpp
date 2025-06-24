#include "BlockClass.hpp"
#include "FunctionAnalyzer.hpp"

Block::Block(BasicBlock* el, FunctionAnalyzer* owner): el(el), owner(owner) {}

std::string Block::getName() const {
    return el->getName().str();
}

void Block::recognize() {
    errs() << "recognize()\n";

    if (llvm::Loop* L = getOwner()->getLoopInfo()->getLoopFor(el)) {
        ownedByLoop = L;

        // se è un loop header no problemi
        if (L->getHeader() == el) {
            type = BlockTypology::LoopHeader;
            return;
        }

        // BEGIN isLoopLatch ?
        if (el == L->getLoopLatch()) {
            type = BlockTypology::LoopLatch;
            return;
        }

        llvm::SmallVector<llvm::BasicBlock*, 4> latches;
        L->getLoopLatches(latches);
        for (llvm::BasicBlock* latch : latches) {
            if (latch == el) {
                type = BlockTypology::LoopLatch;
                return;
            }
        }
        // END isLoopLatch

        // BEGIN isExitBlock
        if (el == L->getExitBlock()) {
            type = BlockTypology::LoopExit;
            return;
        }

        llvm::SmallVector<llvm::BasicBlock*, 4> exists;
        L->getExitBlocks(exists);
        for (llvm::BasicBlock* exit : exists) {
            if (exit == el) {
                type = BlockTypology::LoopExit;
                return;
            }
        }
        // END isExitBlock
    }

    Instruction* term = el->getTerminator();
    if (dyn_cast<BranchInst>(term)) {
        type = BlockTypology::StandardFork;
        return;
    } else if (dyn_cast<SwitchInst>(term)) {
        type = BlockTypology::StandardFork;
        return;
    }

    if (!llvm::pred_empty(el) && el->getUniquePredecessor()) {
        type = BlockTypology::StandardMerge;
        return;
    }

    type = BlockTypology::SimpleBlock;
}

bool Block::accept(BlockVisitor& visitor) {
    switch (type) {
    case BlockTypology::LoopHeader:
        visitor.visitLoopHeader(this);
        break;
    case BlockTypology::LoopLatch:
        visitor.visitLoopLatch(this);
        break;
    case BlockTypology::LoopExit:
        visitor.visitLoopExit(this);
        break;
    case BlockTypology::StandardFork:
        visitor.visitStandardFork(this);
        break;
    case BlockTypology::InterLoopFork:
        visitor.visitInterLoopFork(this);
        break;
    case BlockTypology::StandardMerge:
        visitor.visitStandardMerge(this);
        break;
    case BlockTypology::SimpleBlock:
        visitor.visitSimple(this);
        break;
    default:
        return false;
    }
    return true;
}

llvm::Loop* Block::getLoop() {
    return ownedByLoop;
} 

Scope* Block::emplaceScope(Scope* parent) {
    scope = std::make_unique<Scope>(parent);
    return scope.get();
}


void Block::setNumLoopLatches(llvm::Loop* L) {
    llvm::SmallVector<llvm::BasicBlock*, 4> latches;
    L->getLoopLatches(latches);
    num_latches = latches.size();
}

bool Block::isLoopWholeAnalyzed() {
    return num_latches == 0;
}

void Block::decrRemainingLatches() {
    num_latches--;
}


void Block::setNumBranches(short nb) {
    num_branches = nb;
}

void Block::decrRemainingBranches() {
    num_branches--;
}

bool Block::isForkWholeAnalyzed() {
    return num_branches == 0;
}


void Block::rescaleLoopHeaderScope(Scope* latchScope) {

    // 1 mergia il vecchio parentScope con il latchScope. Questo diventerà il nuovo parentScope

    // rianalizza le istruzioni, soprattutto i phi nodes in modo completo

}

FunctionAnalyzer* Block::getOwner() {
    return owner;
}

BlockTypology Block::getType() {
    return type;
}

void Block::setType(BlockTypology t) {
    type = t;
}

Scope* Block::getScope() {
    return scope.get();
}

BasicBlock* Block::getLLVMBasicBlock() {
    return el;
}

void Block::setIterBounds(std::pair<u_int64_t, u_int64_t> IB) {
    bounds = IB;
}

std::pair<u_int64_t, u_int64_t> Block::getIterBounds() {
    return bounds;
}

Block* Block::findNearestDominatingParent() const {
    // Prendo il DominatorTree dal FunctionAnalyzer (supponendo sia esposto)
    auto &DT = owner->getDominatorTree();

    // Nodo DT del BB corrente
    auto *node = DT.getNode(el);
    if (!node) return nullptr;

    // Risalgo verso l'IDom (immediate dominator) finché trovo un Block* wrappato
    while ((node = node->getIDom())) {
        llvm::BasicBlock *domBB = node->getBlock();
        if (Block* parent = owner->getBlockByLLVMBasicBlock(domBB)) {
            return parent;
        }
    }
    return nullptr;
}