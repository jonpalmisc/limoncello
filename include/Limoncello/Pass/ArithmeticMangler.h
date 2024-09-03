//===-- Pass/ArithmeticMangler.h - Arithmetic-mangling pass ---------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#ifndef LIMONCELLO_PASS_ARITHMETICMANGLER_H
#define LIMONCELLO_PASS_ARITHMETICMANGLER_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/NoFolder.h>
#include <llvm/IR/PassManager.h>

/// Instruction visitor which mangles arithmetic expressions.
///
/// The visit functions will return a new (top-level) instruction if the
/// instruction visited can be replaced with a MBA expression; other
/// instructions will have already been created and parented to the returned
/// instruction in this case. The caller is responsible for replacing the
/// originally visited instruction with the returned result.
///
/// This is overwhelmingly NOT read-only and should be used with care. It WILL
/// mess with the IR and will NOT clean up after itself!
class ManglingVisitor
    : public llvm::InstVisitor<ManglingVisitor, llvm::Instruction *> {
  /// Internal IR builder.
  ///
  /// It is critically important that it is of the "NoFolder" variety as LLVM
  /// is capable of simplifying some MBA expressions on its own before the
  /// optimizer is even invoked.
  llvm::IRBuilder<llvm::NoFolder> m_builder;

public:
  explicit ManglingVisitor(llvm::BasicBlock *block);

  /// Set the insertion point of the internal IR builder.
  ///
  /// This should be called every time a new instruction is inspected.
  void setInsertPoint(llvm::Instruction *inst);

  llvm::Instruction *visitAdd(llvm::BinaryOperator &addOp);
  llvm::Instruction *visitSub(llvm::BinaryOperator &subOp);
  llvm::Instruction *visitAnd(llvm::BinaryOperator &andOp);
  llvm::Instruction *visitOr(llvm::BinaryOperator &orOp);
  llvm::Instruction *visitXor(llvm::BinaryOperator &xorOp);

  static llvm::Instruction *visitInstruction(llvm::Instruction &) {
    return nullptr;
  }
};

/// Pass for replacing normal arithmetic expressions with equivalent mixed
/// boolean-arithmetic expressions.
class ArithmeticManglerPass
    : public llvm::PassInfoMixin<ArithmeticManglerPass> {

  /// Indicates if an operation can be mangled by this pass.
  static bool canMangleOperation(llvm::BinaryOperator const *op);

  /// Create a stub function to perform \p binaryOp between \p lhs and \p rhs.
  static llvm::Function *createBinaryOpStub(llvm::Module *module,
                                            llvm::BinaryOperator *binaryOp,
                                            llvm::Value *lhs, llvm::Value *rhs);

  /// Replace all (obfuscatable) arithmetic expressions in \p func with calls
  /// to generated mixed boolean-arithmetic stub functions.
  ///
  /// Any stub functions created will be inserted into \p stubs.
  static void insertStubs(llvm::Function &func,
                          std::vector<llvm::Function *> &stubs);

public:
  static llvm::PreservedAnalyses run(llvm::Module &module,
                                     llvm::ModuleAnalysisManager &);
  static bool isRequired() { return true; }
};

#endif
