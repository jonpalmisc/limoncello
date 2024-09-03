//===-- Pass/Bloater.h - Function-bloating pass ---------------------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#ifndef LIMONCELLO_PASS_BLOATER_H
#define LIMONCELLO_PASS_BLOATER_H

#include <llvm/IR/PassManager.h>

/// Pass for performing function bloating.
class BloaterPass : public llvm::PassInfoMixin<BloaterPass> {
  /// Get the opaque global variable used by the bloater.
  static llvm::Constant *getOpaqueGlobal(llvm::Module &module);

  /// Get the opaque "always true" function.
  static llvm::Function *getOpaqueTrueFunction(llvm::Module &module);

  /// Perform bloating on a function. Does NOT leave the function in a sound
  /// state, i.e. SSA repairs, etc. will still need to be done after.
  static void bloatFunction(llvm::Function &func);

public:
  static llvm::PreservedAnalyses run(llvm::Module &module,
                                     llvm::ModuleAnalysisManager &);
  static bool isRequired() { return true; }
};

#endif
