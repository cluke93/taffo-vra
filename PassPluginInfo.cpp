#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "VRAPass.h"

using namespace llvm;

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo()
{
  return {
      LLVM_PLUGIN_API_VERSION,
      "Hello",
      "1.0",
      [](PassBuilder &PB) {
        PB.registerPipelineParsingCallback(
            [](StringRef Name, ModulePassManager &PM, ArrayRef<PassBuilder::PipelineElement>) {
              if (Name == "vra") {
                PM.addPass(VRAPass());
                return true;
              }

              return false;
            });
      }};
}
