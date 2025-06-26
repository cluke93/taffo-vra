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
                emplaceFunctionScope(FAN.getName(), FAN.getScope());

                FoundVisitableFunction = true;
            // }
        }

        if (!FoundVisitableFunction)
            LLVM_DEBUG(dbgs() << DEBUG_HEAD << " No visitable functions found.\n");
    }

    void VRAPass::setGlobalScope() {

        auto global = std::make_unique<Scope>(nullptr);

        for (auto& gv : M->globals()) {
            std::string name = gv.getName().str();

            if (!gv.hasInitializer()) {
                //TODO: Se non ha valore iniziale, imposta range "indefinito"
                continue;
            }

            auto* initializer = gv.getInitializer();

            if (auto* cInt = llvm::dyn_cast<llvm::ConstantInt>(initializer)) {
                float val = static_cast<float>(cInt->getSExtValue());

                auto op = std::make_unique<Operand>(name, Range(val, val, true), VarType::Local);

                global->addOperand(std::move(op));

            } else if (auto* cFP = llvm::dyn_cast<llvm::ConstantFP>(initializer)) {
                float val = cFP->getValueAPF().convertToFloat();
                auto op = std::make_unique<Operand>(name, Range(val, val, true), VarType::Local);
                global->addOperand(std::move(op));

            } else {
                // TODO: Gestisci altri tipi se servono (array, struct...)
            }

        }

        globalScope = std::move(global);
    }
    
}

Scope* VRAPass::getGlobalScope() {
    return globalScope.get();
}

ModuleAnalysisManager* VRAPass::getMAM() {
    return MAM;
}

void VRAPass::emplaceFunctionScope(const std::string& fname, Scope* fscope) {
    functionScopes.emplace(fname, fscope);
}

Scope* VRAPass::getFunctionScope(const std::string& fname) const {
    auto it = functionScopes.find(fname);
    return it == functionScopes.end() ? nullptr : it->second;
}

#undef DEBUG_TYPE