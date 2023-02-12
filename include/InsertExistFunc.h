//==============================================================================
// FILE:
//    InsertExistFunc.h
//
// DESCRIPTION:
//    Declares the InsertExistFunc pass for the new and the legacy pass managers.
//
// License: MIT
//==============================================================================
#ifndef LLVM_TUTOR_INSTRUMENT_BASIC_H
#define LLVM_TUTOR_INSTRUMENT_BASIC_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
struct InsertExistFunc : public llvm::PassInfoMixin<InsertExistFunc> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &);
  bool runOnModule(llvm::Module &M);
};

//------------------------------------------------------------------------------
// Legacy PM interface
//------------------------------------------------------------------------------
struct LegacyInsertExistFunc : public llvm::ModulePass {
  static char ID;
  LegacyInsertExistFunc() : ModulePass(ID) {}
  bool runOnModule(llvm::Module &M) override;

  InsertExistFunc Impl;
};

#endif
