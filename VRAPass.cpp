#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>

#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"

#include "VRAPass.h"

#define DEBUG_TYPE "vra"

namespace llvm
{

    PreservedAnalyses VRAPass::run(llvm::Module &M, ModuleAnalysisManager &AM)
    {
        this->M = &M;
        MAM = &AM;

        // Prendo il FunctionAnalysisManager dal ModuleAnalysisManager
        this->FAM = &AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

        processModule();

        return PreservedAnalyses::all();
    }

    void VRAPass::processModule()
    {
        bool FoundVisitableFunction = false;
        for (Function &F : M->functions())
        {
            // if (!F.empty() && (PropagateAll || TaffoInfo::getInstance().isStartingPoint(F))) {

            if (F.getName() != "main")
                continue;

            // 1) predispongo lo scope
            scope_ = std::make_unique<Scope>(nullptr);

            // 2) preparo l'analyzer
            RangeDataflowAnalyzer solver(F);

            // 3) prendo LoopInfo e ScalarEvolution per F
            auto &LI = FAM->getResult<LoopAnalysis>(F);
            auto &SE = FAM->getResult<ScalarEvolutionAnalysis>(F);

            // 4) eseguo il run “SCEV-aware”
            solver.runWithSCEV(F, LI, SE);

            // 5) popolo lo scope e stampo
            solver.populateScope(scope_.get());
            scope_->dump();

            // 6) stampo anche il range di ritorno
            for (BasicBlock &B : F)
            {
                if (auto *RI = dyn_cast<ReturnInst>(B.getTerminator()))
                {
                    if (Value *RV = RI->getReturnValue())
                    {
                        Range r = solver.getValueRange(RV);
                        errs() << "Var " << F.getName() << " with range ( "
                                << r.min << ", " << r.max << ")\n";
                    }
                }
            }
            // }
        }

        if (!FoundVisitableFunction)
            LLVM_DEBUG(dbgs() << DEBUG_HEAD << " No visitable functions found.\n");
    }

    ModuleAnalysisManager *VRAPass::getMAM()
    {
        return MAM;
    }
}

#undef DEBUG_TYPE