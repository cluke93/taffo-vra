#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>

#include "VRAPass.h"

#define DEBUG_TYPE "vra"

namespace llvm
{

    PreservedAnalyses VRAPass::run(llvm::Module& M, ModuleAnalysisManager& AM) {
        this->M = &M;
        MAM = &AM;

        // forse un super global scope
        setGlobalScope();

        processModule();

        return PreservedAnalyses::all();
    }

    void VRAPass::processModule() {
        bool FoundVisitableFunction = false;
        for (Function& F : M->functions()) {
            // if (!F.empty() && (PropagateAll || TaffoInfo::getInstance().isStartingPoint(F))) {

                FunctionAnalyzer FAN = FunctionAnalyzer(&F, this);
                FAN.analyze();
                FoundVisitableFunction = true;
            // }
        }

        if (!FoundVisitableFunction)
            LLVM_DEBUG(dbgs() << DEBUG_HEAD << " No visitable functions found.\n");
    }

    void VRAPass::setGlobalScope() {

        auto global = std::make_unique<Scope>(nullptr, false);

        for (auto& gv : M->globals()) {
            std::string name = gv.getName().str();

            if (!gv.hasInitializer()) {
                //TODO: Se non ha valore iniziale, imposta range "indefinito"
                continue;
            }

            auto* initializer = gv.getInitializer();

            if (auto* cInt = llvm::dyn_cast<llvm::ConstantInt>(initializer)) {
                float val = static_cast<float>(cInt->getSExtValue());
                global->addVar(name, Range(val, val, true));

            } else if (auto* cFP = llvm::dyn_cast<llvm::ConstantFP>(initializer)) {
                float val = cFP->getValueAPF().convertToFloat();
                global->addVar(name, Range(val, val, true));

            } else {
                // TODO: Gestisci altri tipi se servono (array, struct...)
            }

        }

        globalScope = std::move(global);
    }
    
}

#undef DEBUG_TYPE