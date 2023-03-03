//========================================================================
// FILE:
//    InsertExistFunc.cpp
//
// DESCRIPTION:
//    모듈 내의 함수를 한 번 순회하여 우리가 원하는 이름을 찾아 그 함수를 저장
//    그 뒤 각 branch instruction 조우 시 그 함수를 호출하는 패스
//    여기에선 print라는 함수를 찾지만, 원하는 이름으로 수정 가능
//
// USAGE:
//    1. Legacy pass manager:
//      $ opt -load <BUILD_DIR>/lib/libInsertExistFunc.so `\`
//        --legacy-insert-exist-func <bitcode-file>
//    2. New pass maanger:
//      $ opt -load-pass-plugin <BUILD_DIR>/lib/libInsertExistFunc.so `\`
//        -passes=-"insert-exist-func" <bitcode-file>
//
// License: MIT
//========================================================================
#include "InsertExistFunc.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/IR/IRBuilder.h"

#include <vector>

#define DEBUG_TYPE "insert-exist-func"

//-----------------------------------------------------------------------------
// InsertExistFunc implementation
//-----------------------------------------------------------------------------

using namespace llvm;

static char ID;
Function *monitor;

bool InsertExistFunc::runOnModule(Module &M) {
    bool FindExistFunc = false;
    LLVM_DEBUG(dbgs() << "====----- Entered Module " << M.getName()
                      << "\n");

    int counter = 0;

    for(Module::iterator F = M.begin(), E = M.end(); F!= E; ++F)
    {
        LLVM_DEBUG(dbgs() << "Function name: " << F->getName()
                      << "\n");
        if(F->getName() == "print"){
            monitor = cast<Function>(F);
            FindExistFunc = true;
            continue;
        }

        for(Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB)
        {
            for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
            {
                if(isa<BranchInst>(&(*BI)) )
                {
                    LLVM_DEBUG(dbgs() << "found a brach instruction!"
                      << "\n");
                    ArrayRef< Value* > arguments(ConstantInt::get(Type::getInt32Ty(M.getContext()), counter, true));
                    counter++;
                    Instruction *newInst = CallInst::Create(monitor, arguments, "");
                    BB->getInstList().insert(BI, newInst); 
                    LLVM_DEBUG(dbgs() << "Inserted the function!"
                      << "\n");
                }

            }
        }
    }

    return FindExistFunc;
}

PreservedAnalyses InsertExistFunc::run(llvm::Module &M,
                                       llvm::ModuleAnalysisManager &) {
  bool Changed =  runOnModule(M);

  return (Changed ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all());
}

bool LegacyInsertExistFunc::runOnModule(llvm::Module &M) {
  bool Changed = Impl.runOnModule(M);

  return Changed;
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getInsertExistFuncPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "insert-exist-func", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "insert-exist-func") {
                    MPM.addPass(InsertExistFunc());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getInsertExistFuncPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
char LegacyInsertExistFunc::ID = 0;

// Register the pass - required for (among others) opt
static RegisterPass<LegacyInsertExistFunc>
    X(/*PassArg=*/"legacy-insert-exist-func", /*Name=*/"LegacyInsertExistFunc",
      /*CFGOnly=*/false, /*is_analysis=*/false);
