#ifndef PASS_VRA_H
#define PASS_VRA_H

#include "RangeDataFlowAnalyzer.hpp"

#define DEBUG_HEAD "[TAFFO][VRA]"

namespace llvm {

    /*
     * Definition of the class for our hello world pass.
     */
    class VRAPass : public PassInfoMixin<VRAPass> {
    public:

        PreservedAnalyses run(Module& M, ModuleAnalysisManager& AM);

        ModuleAnalysisManager* getMAM();

    protected:

        void processModule();

    private:

        /// Module of this pass
        Module* M;

        /// @brief Global module analysis manager
        ModuleAnalysisManager* MAM;

        std::unique_ptr<Scope> scope_;

        FunctionAnalysisManager* FAM;
        
    };
    
} // namespace llvm


#endif