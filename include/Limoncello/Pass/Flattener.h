//===-- Pass/Flattener.h - Control flow flattening pass -------------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#ifndef LIMONCELLO_PASS_FLATTENER_H
#define LIMONCELLO_PASS_FLATTENER_H

#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>

/// Intermediary "state machine" structure used to simplify the control flow
/// flattening process.
class StateMachine {
  llvm::BasicBlock *m_entryBlock = nullptr;
  llvm::SwitchInst *m_switchInst = nullptr;
  llvm::Value *m_stateVar = nullptr;
  llvm::BasicBlock *m_switchBlock = nullptr;
  llvm::BasicBlock *m_defaultBlock = nullptr;
  llvm::BasicBlock *m_endBlock = nullptr;

  static bool shouldIgnoreTerminator(llvm::Instruction *instruction);

  void rewriteBlock(llvm::BasicBlock *block);
  void rewriteBranch(llvm::BranchInst *branchInst);

public:
  explicit StateMachine(llvm::BasicBlock *entryBlock);

  void addState(llvm::BasicBlock *block) const;
  void finalize(llvm::BasicBlock *firstBlock,
                llvm::SmallVector<llvm::BasicBlock *> const &flatteningSet);
};

using BlockPair = std::pair<llvm::BasicBlock *, llvm::BasicBlock *>;

/// Pass for performing control flow flattening.
class FlattenerPass : public llvm::PassInfoMixin<FlattenerPass> {
  /// Get the set of blocks in \p func which should be flattened.
  static llvm::SmallVector<llvm::BasicBlock *>
  getFlatteningSet(llvm::Function &func);

  /// Remove the terminator from \p block, including the condition instruction
  /// if the terminator is a conditional branch.
  static BlockPair splitConditionalPart(llvm::BasicBlock *block);

public:
  static llvm::PreservedAnalyses run(llvm::Function &func,
                                     llvm::FunctionAnalysisManager &);
  static bool isRequired() { return true; }
};

#endif
