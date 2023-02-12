
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

#include "llvm/IR/IRBuilder.h"

#include <vector>

using namespace llvm;

namespace{
struct ir_instrumentation : public ModulePass{
    static char ID;
    Function *monitor;

    ir_instrumentation() : ModulePass(ID) {}

    virtual bool runOnModule(Module &M)
    {
        errs() << "====----- Entered Module " << M.getName() << ".\n";

        int counter = 0;

        for(Module::iterator F = M.begin(), E = M.end(); F!= E; ++F)
        {
            errs() << "Function name: " << F->getName() << ".\n";
            if(F->getName() == "print"){
                monitor = cast<Function>(F);
                continue;
            }

            for(Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB)
            {
                for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
                {
                    if(isa<BranchInst>(&(*BI)) )
                    {
                        errs() << "found a brach instruction!\n";
                        ArrayRef< Value* > arguments(ConstantInt::get(Type::getInt32Ty(M.getContext()), counter, true));
                        counter++;
                        Instruction *newInst = CallInst::Create(monitor, arguments, "");
                        BB->getInstList().insert(BI, newInst); 
                        errs() << "Inserted the function!\n";
                    }

                }
            }
        }

        return true;
    }
};
char ir_instrumentation::ID = 0;
static RegisterPass<ir_instrumentation> X("ir-instrumentation", "LLVM IR Instrumentation Pass");

}
