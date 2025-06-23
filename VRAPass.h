#ifndef PASS_VRA_H
#define PASS_VRA_H

#include "FunctionAnalyzer.hpp"

#define DEBUG_HEAD "[TAFFO][VRA]"

namespace llvm {

    /*
     * Definition of the class for our hello world pass.
     */
    class VRAPass : public PassInfoMixin<VRAPass> {
    public:

        PreservedAnalyses run(Module& M, ModuleAnalysisManager& AM);

        void addFunctionScope(std::unique_ptr<Scope> fnScope);

    protected:

    
        void VRAPass::setGlobalScope();

        void processModule();

    private:
        

        /// @brief Scope for global variables
        std::unique_ptr<Scope> globalScope;

        /// @brief Scope for each function computed by VRA
        std::vector<std::unique_ptr<Scope>> functionScopes;

        /// Module of this pass
        Module* M;

        /// @brief Global module analysis manager
        ModuleAnalysisManager* MAM;
        
    };
    
} // namespace llvm


#endif