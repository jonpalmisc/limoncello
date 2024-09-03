//===-- Pass/ConstantMangler.h - Constant-mangling pass -------------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#ifndef LIMONCELLO_PASS_CONSTANTMANGLER_H
#define LIMONCELLO_PASS_CONSTANTMANGLER_H

#include <llvm/IR/PassManager.h>

class ConstantManglerPass : public llvm::PassInfoMixin<ConstantManglerPass> {
  /// Indicates if an instruction can be descended into and have it's operands
  /// modified as part of this pass.
  static bool canMangleInstruction(llvm::Instruction const &insn);

  /// Mangle all of the constants in \p func.
  static bool mangleFunctionConstants(llvm::Function &func);

public:
  static llvm::PreservedAnalyses run(llvm::Module &module,
                                     llvm::ModuleAnalysisManager &);
  static bool isRequired() { return true; }
};

#endif
