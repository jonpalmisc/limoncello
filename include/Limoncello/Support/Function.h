//===-- Support/Function.h - Function-related helpers ---------------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#ifndef LIMONCELLO_SUPPORT_FUNCTION_H
#define LIMONCELLO_SUPPORT_FUNCTION_H

#include <llvm/IR/IRBuilder.h>

/// Make \p function local to \p module (optionally with a \p newName) by
/// cloning it into a new function in \p module.
///
/// The name of the existing function will be used if \p newName is empty.
llvm::Function *localizeFunction(llvm::Module &module, llvm::Function &function,
                                 llvm::StringRef newName = "");

using BuildStubCallback =
    std::function<void(llvm::Function *, llvm::IRBuilder<> &)>;

/// Create a "stub function" which will always be inlined and never be
/// optimized with the given \p name and \p type inside of \p module.
///
/// The body of the function (expected to be one block) will be populated by
/// the \p build callback.
llvm::Function *createStubFunction(llvm::Module &module, llvm::StringRef name,
                                   llvm::FunctionType *type,
                                   BuildStubCallback build);

/// Tells whether the value of \p inst escapes its parent block.
bool valueEscapesLocalBlock(llvm::Instruction &value);

/// Perform repairs to the IR for \p func as to not break SSA rules.
void repairSSA(llvm::Function &func);

#endif
