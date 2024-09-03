//===-- Pass/StringObfuscator.h - String obfuscation pass -----------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#ifndef LIMONCELLO_PASS_STRINGOBFUSCATOR_H
#define LIMONCELLO_PASS_STRINGOBFUSCATOR_H

#include <llvm/IR/PassManager.h>

/// Record of a string that has been obfuscated during this pass.
class ObfuscatedStringRecord {
public:
  /// Global variable associated with the string.
  llvm::GlobalVariable *handle;

  /// The size of the string in bytes.
  size_t size;

  /// The XOR key used to obfuscate the string.
  uint8_t key;

  // XXX: This exists purely to appease `clangd` when trying to construct these
  // in place during a call to `emplace_back`.
  ObfuscatedStringRecord(llvm::GlobalVariable *handle, size_t size, uint8_t key)
      : handle(handle), size(size), key(key) {}
};

class StringObfuscatorPass : public llvm::PassInfoMixin<StringObfuscatorPass> {
  /// Get a local copy of the deobfuscation routine.
  ///
  /// Guaranteed to return a valid function (or panic).
  static llvm::Function *getDeobfuscateFunction(llvm::Module &module);

  /// Create a function to deobfuscate all strings in \p records.
  static llvm::Function *createDeobfuscateAllFunction(
      llvm::Module &module, std::vector<ObfuscatedStringRecord> const &records);

  /// Create a new strings constant with the obfuscated version of \p content
  /// by XOR-ing each byte with \p key.
  static llvm::Constant *createObfuscatedString(llvm::LLVMContext &context,
                                                llvm::StringRef content,
                                                uint8_t key);

  /// Obfuscate all globally-defined strings in \p module.
  static std::vector<ObfuscatedStringRecord>
  obfuscateStrings(llvm::Module &module);

public:
  static llvm::PreservedAnalyses run(llvm::Module &module,
                                     llvm::ModuleAnalysisManager &);
  static bool isRequired() { return true; }
};

#endif
