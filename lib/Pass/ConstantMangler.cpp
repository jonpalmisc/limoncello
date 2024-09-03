//===-- Pass/ConstantMangler.cpp - Constant-mangling pass -----------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#include "Limoncello/Pass/ConstantMangler.h"

#include "Limoncello/Config/Config.h"
#include "Limoncello/Support/Module.h"
#include "Limoncello/Support/Random.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/NoFolder.h>

using namespace llvm;

bool ConstantManglerPass::canMangleInstruction(Instruction const &insn) {
  if (isa<CallInst>(insn) || isa<SwitchInst>(insn) ||
      isa<GetElementPtrInst>(insn) || isa<PHINode>(insn) || insn.isAtomic())
    return false;

  return true;
}

bool ConstantManglerPass::mangleFunctionConstants(Function &func) {
  auto module = func.getParent();
  auto int64Ty = Type::getInt64Ty(module->getContext());
  auto opaqueGlobal = getOrInsertGlobal(*module, "__lmcoOpaqueGlobal",
                                        ConstantInt::get(int64Ty, 0));

  bool changed = false;
  for (auto &inst : instructions(func)) {
    if (!canMangleInstruction(inst))
      continue;

    for (auto &op : inst.operands()) {
      if (auto *intOp = dyn_cast<ConstantInt>(op)) {
        auto intType = intOp->getType();
        auto xorKey = getRandomInt64();

        IRBuilder<NoFolder> irb(&inst);
        auto rawOpaqueValue = irb.CreateLoad(int64Ty, opaqueGlobal);
        auto opaqueValue = irb.CreateTrunc(rawOpaqueValue, intType);
        auto mangledConstant =
            ConstantInt::get(intType, intOp->getValue() ^ xorKey);
        auto opacifiedKey =
            irb.CreateOr(opaqueValue, ConstantInt::get(intType, xorKey));
        auto xorExpr = irb.CreateXor(mangledConstant, opacifiedKey);

        op.set(xorExpr);
        changed |= true;
      }
    }
  }

  return changed;
}

PreservedAnalyses ConstantManglerPass::run(Module &module,
                                           ModuleAnalysisManager &) {
  bool changed = false;
  for (auto &func : module.getFunctionList()) {
    if (!Config::get()->constantMangler.shouldRunOnFunction(func))
      continue;

    mangleFunctionConstants(func);
    changed |= true;
  }

  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
