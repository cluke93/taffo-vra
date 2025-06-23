#include "FunctionAnalyzer.hpp"
#include "RangePropagationVisitor.cpp"
#include "VRAPass.h"

FunctionAnalyzer::FunctionAnalyzer(Function* el, llvm::VRAPass* vra_pass) : el(el), vra_pass(vra_pass),  
    FAM(vra_pass->getMAM()->getResult<llvm::FunctionAnalysisManagerModuleProxy>(*el->getParent()).getManager()), 
    SE(FAM.getResult<ScalarEvolutionAnalysis>(*el)),
    DT(&FAM.getResult<DominatorTreeAnalysis>(*el)) {

        loopInfo = &(FAM.getResult<llvm::LoopAnalysis>(*el));
}

/**
 * visitor pattern
 */
void FunctionAnalyzer::analyze() {

    Scope* globalScope = getPass()->getGlobalScope();
    scope = std::make_unique<Scope>(globalScope);

    // recuperare gli argomenti col loro range

    IA = std::make_shared<InstructionAnalyzer>(std::make_shared<RangeHandler>());

    // preparare il blocco entry
    BasicBlock* entryBlock = &el->getEntryBlock();
    workload.push(entryBlock);

    RangePropagationVisitor visitor(this);
    
    while (!workload.empty()) {

        BasicBlock* curBasicBlock = workload.front();
        workload.pop();

        if (contains(curBasicBlock)) continue;

        Block* curBlock = emplaceBlock(curBasicBlock, this);
        
        curBlock->recognize();
        curBlock->accept(visitor);
    }


    // devo capire se la funzione non è void il range dei valori che usciranno

}


std::pair<u_int64_t, u_int64_t> FunctionAnalyzer::getLoopIterBounds(llvm::Loop* L) {
    std::string indent(breadcrumb.size() + 1, '-');

    u_int64_t min_iter = 0;
    u_int64_t max_iter = 0;

    // Trip count esatto (se disponibile)
    unsigned exactTripCount = SE.getSmallConstantTripCount(L);
    min_iter = exactTripCount;

    const SCEV* backEdgeCount = SE.getBackedgeTakenCount(L);
    if (backEdgeCount && !isa<SCEVCouldNotCompute>(backEdgeCount)) {
        if (auto *constTrip = dyn_cast<SCEVConstant>(backEdgeCount)) {
            const APInt &val = constTrip->getValue()->getValue();
            max_iter = val.getZExtValue();
        } else {
            outs() << indent << " -Trip count non costante\n";
            // fallback conservativo
            max_iter = 100;
        }
    } else {
        outs() << indent << " -Trip count uncomputable\n";
        max_iter = 100;
    }

    outs() << indent << " -Loop bounds: min_iter = " << min_iter << ", max_iter = " << max_iter << "\n";

    return {min_iter, max_iter};
}

bool FunctionAnalyzer::contains(BasicBlock* bb) const {
    return std::any_of(
        ownedBlocks.begin(),
        ownedBlocks.end(),
        [bb](const std::unique_ptr<Block>& b){
            return b->getLLVMBasicBlock() == bb;
        }
    );
}



Block* FunctionAnalyzer::addBlock(std::unique_ptr<Block> block) {
    Block* ptr = block.get();
    ownedBlocks.push_back(std::move(block));
    return ptr;
}

template<typename... Args>
Block* FunctionAnalyzer::emplaceBlock(Args&&... args) {
    // Costruisce il Block direttamente dentro ownedBlocks
    ownedBlocks.emplace_back(std::make_unique<Block>(std::forward<Args>(args)...));
    return ownedBlocks.back().get();
}

void FunctionAnalyzer::addBreadcrumb(BlockAggregation* agg) {
    breadcrumb.push_back(agg);
}

BlockAggregation* FunctionAnalyzer::emplaceBreadcrumb(AggregationType type, Block* ref) {
    // Alloca dinamicamente l’aggregazione
    auto* agg = new BlockAggregation(type, ref);
    breadcrumb.push_back(agg);
    return agg;
}

void FunctionAnalyzer::popBreadcrumb() {
    breadcrumb.pop_back();
}



/**
 * Gestione loop header
 */
void FunctionAnalyzer::initLoop(Block* header) {
    errs() << "initLoop(exit: " << header->getName() << ")";
    
    header->setNumLoopLatches(header->getLoop());

    Scope* parentScope = nullptr;
    if (Block* parentBlock = header->findNearestDominatingParent()) {
        parentScope = parentBlock->getScope();
    }
    
    Scope* scope = header->emplaceScope(parentScope);

    setAnalysisBound();

    IA->loadBlock(header);

    // prima i phi nodes (che comunque dovrebbero essere tutti all'inizio)
    for (Instruction& I : *header->getLLVMBasicBlock()) {
        IA->analyzePHINodesLoopHeader(&I);
    }

    // ora le expressions che modificheranno lo scope
    for (Instruction& I : *header->getLLVMBasicBlock()) {
        IA->analyzeExpressionNodes(&I);
    }

    IA->freeBlock();

    header->setIterBounds(getLoopIterBounds(header->getLoop()));

    Instruction* term = header->getLLVMBasicBlock()->getTerminator();
    if (auto* br = dyn_cast<BranchInst>(term)) {

        for (unsigned int i = 0; i<br->getNumSuccessors(); i++) {
            if (llvm::Loop* L = getLoopInfo()->getLoopFor(br->getSuccessor(i))) {
                if (L == header->getLoop()) { //solo il loop di appartenenza della funzione, altrimenti tutti i branch di loop annidati possono dare problemi
                    enqueueBlock(br->getSuccessor(i));
                    return;
                }
                
            }
        }
        
    }

}

void FunctionAnalyzer::initStandardFork(Block* fork) {
    errs() << "initStandardFork(exit: " << fork->getName() << ")";

    BasicBlock* el = fork->getLLVMBasicBlock();

    Scope* parentScope = nullptr;
    if (Block* parentBlock = fork->findNearestDominatingParent()) {
        parentScope = parentBlock->getScope();
    }
    
    Scope* scope = fork->emplaceScope(parentScope);

    setAnalysisBound();

    IA->loadBlock(fork);

    // prima i phi nodes (che comunque dovrebbero essere tutti all'inizio)
    for (Instruction& I : *el) {
        IA->analyzePHINodes(&I);
    }

    // ora le expressions che modificheranno lo scope
    for (Instruction& I : *el) {
        IA->analyzeExpressionNodes(&I);
    }

    IA->freeBlock();

    // LAST PART: next blocks enqueue
    short num_branches = 0;

    Instruction* term = el->getTerminator();
    if (auto* br = dyn_cast<BranchInst>(term)) {
        
        for (unsigned int i = 0; i<br->getNumSuccessors(); i++) {
            auto* succ = br->getSuccessor(i);

            // il successore non esce dal loop corrente
            if (isNotBreakLoopKeyword(el, succ)) {
                enqueueBlock(br->getSuccessor(i));
                num_branches++;
            } else {
                // il successore esce dal loop corrente?
                // il fork diventa InterLoop (ne basta uno), non accodo il blocco (sarà analizzato con i loop exit)
                //TODO: break dentro fork innestati

                fork->setType(BlockTypology::InterLoopFork);
            }
        }
        

    } else if (auto* sw = dyn_cast<SwitchInst>(term)) {
        
        for (unsigned int i = 1; i <= sw->getNumCases(); i++) {
            enqueueBlock(br->getSuccessor(i));
            num_branches++;
        }

        //last op is default
        enqueueBlock(br->getSuccessor(0));
        num_branches++;
    }

    fork->setNumBranches(num_branches);
}

void FunctionAnalyzer::processSimpleBlock(Block* block) {
    errs() << "processSimpleBlock(exit: " << block->getName() << ")";

    BasicBlock* el = block->getLLVMBasicBlock();

    Scope* parentScope = nullptr;
    if (Block* parentBlock = block->findNearestDominatingParent()) {
        parentScope = parentBlock->getScope();
    }
    
    Scope* scope = block->emplaceScope(parentScope);

    setAnalysisBound();

    IA->loadBlock(block);

    for (Instruction& I : *el) {
        IA->analyzeExpressionNodes(&I);
    }

    IA->freeBlock();

    Instruction* term = el->getTerminator();
    if (auto* br = dyn_cast<BranchInst>(term)) {

        auto* succ = br->getSuccessor(0);
        if (isNotBreakLoopKeyword(el, succ)) {
            enqueueBlock(br->getSuccessor(0));
        }
    }
}

void FunctionAnalyzer::processLoopLatch(Block* latch) {
    errs() << "processLoopLatch(exit: " << latch->getName() << ")";

    // risalire al breadcrumb del loop corrente
    Block* loopHeaderBlock = getLoopHeaderFromBreadcrumb();
    loopHeaderBlock->decrRemainingLatches();

    BasicBlock* el = latch->getLLVMBasicBlock();

    Scope* parentScope = nullptr;
    if (Block* parentBlock = latch->findNearestDominatingParent()) {
        parentScope = parentBlock->getScope();
    }
    
    Scope* scope = latch->emplaceScope(parentScope);

    setAnalysisBound();

    IA->loadBlock(latch);

    for (Instruction& I : *el) {
        IA->analyzeExpressionNodes(&I);
    }

    IA->freeBlock();

    // se non analizzato tutto, chiudi qui
    if (!loopHeaderBlock->isLoopWholeAnalyzed()) return;

    loopHeaderBlock->rescaleLoopHeaderScope(scope);

    if (BasicBlock* EB = loopHeaderBlock->getLoop()->getExitBlock()) {
        enqueueBlock(EB);
        return;
    }

    llvm::SmallVector<llvm::BasicBlock*, 4> latches;
    loopHeaderBlock->getLoop()->getExitBlocks(latches);
    for (llvm::BasicBlock* latch : latches) {
        enqueueBlock(latch);
    }

    popBreadcrumb();
}

void FunctionAnalyzer::handleStandardMerge(Block* merge) {
    errs() << "handleStandardMerge(exit: " << merge->getName() << ")";

    Block* forkBlock = getForkFromBreadcrumb();
    forkBlock->decrRemainingBranches();

    if (!forkBlock->isForkWholeAnalyzed()) return;

    BasicBlock* el = merge->getLLVMBasicBlock();

    Scope* parentScope = nullptr;
    if (BasicBlock* bb = merge->getLLVMBasicBlock()->getUniquePredecessor()) {
        parentScope = getBlockByLLVMBasicBlock(bb)->getScope();
    } else {

        std::unique_ptr<Scope> tempParentScopeHolder = nullptr;
        for (llvm::BasicBlock *pred : llvm::predecessors(merge->getLLVMBasicBlock())) {
            if (tempParentScopeHolder == nullptr) {
                std::make_unique<Scope>(getBlockByLLVMBasicBlock(pred)->getScope());
            } else {
                tempParentScopeHolder->mergeWith(getBlockByLLVMBasicBlock(pred)->getScope());
            }
        }
        parentScope = tempParentScopeHolder.get();
    }

    Scope* scope = merge->emplaceScope(parentScope);

    setAnalysisBound();

    IA->loadBlock(merge);

    for (Instruction& I : *el) {
        IA->analyzePHINodes(&I);
    }

    for (Instruction& I : *el) {
        IA->analyzeExpressionNodes(&I);
    }

    IA->freeBlock();

    // pulisci la breadcrumb ora che il fork è terminato
    popBreadcrumb();

    //FINAL CHECK: IS THIS MERGE BLOCK ALSO THE BEGIN OF ANOTHER FORK?
    short num_branches = 0;

    Instruction* term = el->getTerminator();
    if (auto* br = dyn_cast<BranchInst>(term)) {
        
        if (br->getNumSuccessors() == 1) {
            // NON è l'inizio di un altro fork, accodo e termino qui
            enqueueBlock(br->getSuccessor(0));
            return;
        } else {
            for (unsigned int i = 0; i<br->getNumSuccessors(); i++) {
                auto* succ = br->getSuccessor(i);

                // il successore non esce dal loop corrente
                if (isNotBreakLoopKeyword(el, succ)) {
                    enqueueBlock(br->getSuccessor(i));
                    num_branches++;
                } else {
                    // il successore esce dal loop corrente?
                    // il fork diventa InterLoop (ne basta uno), non accodo il blocco (sarà analizzato con i loop exit)
                    //TODO: break dentro fork innestati

                    merge->setType(BlockTypology::InterLoopFork);
                }
            }
        }
    } else if (auto* sw = dyn_cast<SwitchInst>(term)) {
        
        for (unsigned int i = 1; i <= sw->getNumCases(); i++) {
            enqueueBlock(br->getSuccessor(i));
            num_branches++;
        }

        //last op is default
        enqueueBlock(br->getSuccessor(0));
        num_branches++;
    }

    // qui arrivo solo se è l'inizio di un altro fork, quindi cambio il tipo di blocco (l'indicazione precedente del merge non serve più)
    merge->setNumBranches(num_branches);
    merge->setType(BlockTypology::StandardFork);
    emplaceBreadcrumb(AggregationType::Fork, merge);
}

void FunctionAnalyzer::handleLoopExit(Block* exit) {
    errs() << "handleLoopExit(exit: " << exit->getName() << ")";

    BasicBlock* el = exit->getLLVMBasicBlock();

    Scope* parentScope = nullptr;
    if (BasicBlock* bb = exit->getLLVMBasicBlock()->getUniquePredecessor()) {
        parentScope = getBlockByLLVMBasicBlock(bb)->getScope();
    } else {

        std::unique_ptr<Scope> tempParentScopeHolder = nullptr;
        for (llvm::BasicBlock *pred : llvm::predecessors(exit->getLLVMBasicBlock())) {
            if (tempParentScopeHolder == nullptr) {
                std::make_unique<Scope>(getBlockByLLVMBasicBlock(pred)->getScope());
            } else {
                tempParentScopeHolder->mergeWith(getBlockByLLVMBasicBlock(pred)->getScope());
            }
        }
        parentScope = tempParentScopeHolder.get();
    }

    Scope* scope = exit->emplaceScope(parentScope);

    setAnalysisBound();

    //TODO: restore dei min/max iter precedenti al loop
    IA->loadBlock(exit);

    for (Instruction& I : *el) {
        IA->analyzePHINodes(&I);
    }

    for (Instruction& I : *el) {
        IA->analyzeExpressionNodes(&I);
    }

    IA->freeBlock();

    //FINAL CHECK: IS THIS EXIT BLOCK ALSO THE BEGIN OF ANOTHER FORK?
    short num_branches = 0;
    Instruction* term = el->getTerminator();
    if (auto* br = dyn_cast<BranchInst>(term)) {
        
        if (br->getNumSuccessors() == 1) {
            // NON è l'inizio di un altro fork, accodo e termino qui
            enqueueBlock(br->getSuccessor(0));
            return;
        } else {
            for (unsigned int i = 0; i<br->getNumSuccessors(); i++) {
                auto* succ = br->getSuccessor(i);

                // il successore non esce dal loop corrente
                if (isNotBreakLoopKeyword(el, succ)) {
                    enqueueBlock(br->getSuccessor(i));
                    num_branches++;
                } else {
                    // il successore esce dal loop corrente?
                    // il fork diventa InterLoop (ne basta uno), non accodo il blocco (sarà analizzato con i loop exit)
                    //TODO: break dentro fork innestati

                    exit->setType(BlockTypology::InterLoopFork);
                }
            }
        }
    } else if (auto* sw = dyn_cast<SwitchInst>(term)) {
        
        for (unsigned int i = 1; i <= sw->getNumCases(); i++) {
            enqueueBlock(br->getSuccessor(i));
            num_branches++;
        }

        //last op is default
        enqueueBlock(br->getSuccessor(0));
        num_branches++;
    }

    // qui arrivo solo se è l'inizio di un altro fork, quindi cambio il tipo di blocco (l'indicazione precedente del merge non serve più)
    exit->setNumBranches(num_branches);
    exit->setType(BlockTypology::StandardFork);
    emplaceBreadcrumb(AggregationType::Fork, exit);
}

void FunctionAnalyzer::handleReturnBlock(Block* ret) {

    

}

void FunctionAnalyzer::enqueueBlock(BasicBlock* B) {
    workload.push(B);
}

Block* FunctionAnalyzer::getLoopHeaderFromBreadcrumb() const {
    // scorri breadcrumb a ritroso (LIFO)
    for (auto it = breadcrumb.rbegin(); it != breadcrumb.rend(); ++it) {
        BlockAggregation* agg = *it;
        if (agg->type == AggregationType::Loop) {
            return agg->ref;
        }
    }
    return nullptr;
}

Block* FunctionAnalyzer::getForkFromBreadcrumb() const {
    // scorri breadcrumb a ritroso (LIFO)
    for (auto it = breadcrumb.rbegin(); it != breadcrumb.rend(); ++it) {
        BlockAggregation* agg = *it;
        if (agg->type == AggregationType::Fork) {
            return agg->ref;
        }
    }
    return nullptr;
}

Block* FunctionAnalyzer::getBlockByLLVMBasicBlock(BasicBlock* bb) const {
    for (auto const& up : ownedBlocks) {
        if (up->getLLVMBasicBlock() == bb)
            return up.get();
    }
    return nullptr;
}

void FunctionAnalyzer::setAnalysisBound() {
    Block* b = getLoopHeaderFromBreadcrumb();
    if (b) {
        IA->setCurrentIterBounds(b->getIterBounds());
    } else {
        IA->setCurrentIterBounds({1,1});
    }
}