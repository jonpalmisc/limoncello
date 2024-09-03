//===-- Support/Module.h - Module-related helpers -------------------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#ifndef LIMONCELLO_SUPPORT_MODULE_H
#define LIMONCELLO_SUPPORT_MODULE_H

#include <llvm/IR/Module.h>

/// Make \p name unique by "salting" it with a module-specific suffix.
std::string getModuleSpecificName(llvm::Module &module, llvm::StringRef name);

/// Create a new global variable inside \p module called \p name with the given
/// initial \p value.
llvm::Constant *getOrInsertGlobal(llvm::Module &module, llvm::StringRef name,
                                  llvm::Constant *value);

#endif
