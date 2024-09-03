//===-- Pass/ArithmeticMangler.cpp - Arithmetic-mangling pass -------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#include "Limoncello/Pass/ArithmeticMangler.h"

#include "Limoncello/Config/Config.h"
#include "Limoncello/Support/Function.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/NoFolder.h>
#include <llvm/IR/PatternMatch.h>

using namespace llvm;

// TOOD: Replace this with template magic.
#define MATCH(Op, PredicateFn, Lhs, Rhs)                                       \
  (PatternMatch::match(Op, PredicateFn(PatternMatch::m_Value(Lhs),             \
                                       PatternMatch::m_Value(Rhs))))

ManglingVisitor::ManglingVisitor(BasicBlock *block)
    : m_builder(IRBuilder<NoFolder>(block)) {}

void ManglingVisitor::setInsertPoint(Instruction *inst) {
  m_builder.SetInsertPoint(inst);
}

Instruction *ManglingVisitor::visitAdd(BinaryOperator &addOp) {
  Value *lhs, *rhs;
  if (!MATCH(&addOp, PatternMatch::m_Add, lhs, rhs)) {
    return nullptr;
  }

  return BinaryOperator::CreateAdd(m_builder.CreateAnd(lhs, rhs),
                                   m_builder.CreateOr(lhs, rhs));
}

Instruction *ManglingVisitor::visitSub(BinaryOperator &subOp) {
  Value *lhs, *rhs;
  if (!MATCH(&subOp, PatternMatch::m_Sub, lhs, rhs)) {
    return nullptr;
  }

  return BinaryOperator::CreateAdd(
      m_builder.CreateXor(lhs, m_builder.CreateNeg(rhs)),
      m_builder.CreateMul(ConstantInt::get(lhs->getType(), 2),
                          m_builder.CreateAnd(lhs, m_builder.CreateNeg(rhs))));
}

Instruction *ManglingVisitor::visitAnd(BinaryOperator &andOp) {
  Value *lhs, *rhs;
  if (!MATCH(&andOp, PatternMatch::m_And, lhs, rhs)) {
    return nullptr;
  }

  return BinaryOperator::CreateSub(m_builder.CreateAdd(lhs, rhs),
                                   m_builder.CreateOr(lhs, rhs));
}

Instruction *ManglingVisitor::visitOr(BinaryOperator &orOp) {
  Value *lhs, *rhs;
  if (!MATCH(&orOp, PatternMatch::m_Or, lhs, rhs)) {
    return nullptr;
  }

  return BinaryOperator::CreateAdd(
      m_builder.CreateAdd(m_builder.CreateAdd(lhs, rhs),
                          ConstantInt::get(lhs->getType(), 1)),
      m_builder.CreateOr(m_builder.CreateNot(lhs), m_builder.CreateNot(rhs)));
}

Instruction *ManglingVisitor::visitXor(BinaryOperator &xorOp) {
  Value *lhs, *rhs;
  if (!MATCH(&xorOp, PatternMatch::m_Xor, lhs, rhs)) {
    return nullptr;
  }

  return BinaryOperator::CreateSub(m_builder.CreateOr(lhs, rhs),
                                   m_builder.CreateAnd(lhs, rhs));
}

bool ArithmeticManglerPass::canMangleOperation(BinaryOperator const *op) {
  switch (op->getOpcode()) {
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
    return true;
  default:
    return false;
  }
}

Function *ArithmeticManglerPass::createBinaryOpStub(Module *module,
                                                    BinaryOperator *op,
                                                    Value *lhs, Value *rhs) {
  auto type = FunctionType::get(op->getType(), {lhs->getType(), rhs->getType()},
                                /*isVarArg=*/false);
  return createStubFunction(*module, "__lmcoArithmeticStub", type,
                            [=](Function *f, IRBuilder<> &entryBuilder) {
                              entryBuilder.CreateRet(entryBuilder.CreateBinOp(
                                  op->getOpcode(), f->getArg(0), f->getArg(1)));
                            });
}

void ArithmeticManglerPass::insertStubs(Function &func,
                                        std::vector<Function *> &stubs) {
  SmallVector<Instruction *> replacedInstructions;
  for (auto &inst : instructions(func)) {
    auto binaryOp = dyn_cast<BinaryOperator>(&inst);
    if (!binaryOp || !canMangleOperation(binaryOp))
      continue;

    auto lhs = binaryOp->getOperand(0);
    auto rhs = binaryOp->getOperand(1);
    auto stub = createBinaryOpStub(func.getParent(), binaryOp, lhs, rhs);

    IRBuilder<> builder(&inst);
    auto result = builder.CreateCall(stub->getFunctionType(), stub, {lhs, rhs});
    inst.replaceAllUsesWith(result);
    replacedInstructions.emplace_back(&inst);

    stubs.emplace_back(stub);
  }

  for (auto inst : replacedInstructions)
    inst->eraseFromParent();
}

PreservedAnalyses ArithmeticManglerPass::run(Module &module,
                                             ModuleAnalysisManager &) {
  auto config = Config::get();

  // Since the MBA stub functions are created on-demand (and parented to the
  // current module) the module's function list cannot be used directly as the
  // iterator will become invalid.
  //
  // To avoid inevitable crashes due to invalid iterators (and to avoid
  // recursively processing MBA stubs), the original list of functions needs to
  // be backed up.
  std::vector<Function *> originalFunctions;
  originalFunctions.reserve(module.getFunctionList().size());
  std::transform(
      module.getFunctionList().begin(), module.getFunctionList().end(),
      std::back_inserter(originalFunctions), [](Function &f) { return &f; });

  // Iterate through all of the module's functions (that match the filtering
  // criteria for the pass), replacing obfuscatable arithmetic expressions with
  // calls to MBA stub functions.
  std::vector<Function *> stubFunctions;
  for (auto function : originalFunctions) {
    if (!config->arithmeticMangler.shouldRunOnFunction(*function))
      continue;

    insertStubs(*function, stubFunctions);
  }

  // The stub functions created simply extracted the original arithmetic
  // expression into a separate function; MBA obfuscation has not been applied
  // yet.
  //
  // Each stub function needs to be processed and have MBA equivalents of its
  // operations inserted where appropriate.
  for (auto stub : stubFunctions) {
    for (int i = 0; i < config->arithmeticMangler.rounds; ++i) {
      for (auto &block : *stub) {
        ManglingVisitor visitor(&block);

        SmallVector<Instruction *> replacedInstructions;
        for (auto &inst : block) {
          visitor.setInsertPoint(&inst);
          auto replacement = visitor.visit(inst);
          if (!replacement || replacement == &inst)
            continue;

          replacement->takeName(&inst);
          replacement->insertInto(&block, inst.getIterator());
          inst.replaceAllUsesWith(replacement);
          replacedInstructions.push_back(&inst);
        }

        for (auto inst : replacedInstructions)
          inst->eraseFromParent();
      }
    }
  }

  return stubFunctions.empty() ? PreservedAnalyses::none()
                               : PreservedAnalyses::all();
}
