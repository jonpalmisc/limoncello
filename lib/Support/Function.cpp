//===-- Support/Function.cpp - Function-related helpers -------------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#include "Limoncello/Support/Function.h"

#include "Limoncello/Config/Config.h"
#include "Limoncello/Support/Module.h"

#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/Transforms/Utils/ValueMapper.h>

using namespace llvm;

Function *localizeFunction(Module &module, Function &function,
                           StringRef newName) {
  // Default the new local function's name to the external function's name if
  // an override is not provided.
  if (newName.empty())
    newName = function.getName();

  auto localCallee =
      module.getOrInsertFunction(newName, function.getFunctionType());
  auto localFunction = cast<Function>(localCallee.getCallee());

  auto localArg = localFunction->arg_begin();
  auto arg = function.arg_begin();

  ValueToValueMapTy argMap;
  for (auto argsEnd = function.arg_end(); arg != argsEnd; ++localArg, ++arg)
    argMap[&*arg] = &*localArg;

  SmallVector<ReturnInst *, 8> returnValues;
  CloneFunctionInto(localFunction, &function, argMap,
                    CloneFunctionChangeType::DifferentModule, returnValues);

  return localFunction;
}

Function *createStubFunction(Module &module, StringRef name, FunctionType *type,
                             BuildStubCallback build) {
  auto stub = Function::Create(type, Config::get()->getDefaultLinkage(),
                               getModuleSpecificName(module, name), module);
  stub->addFnAttr(Attribute::AlwaysInline);
  stub->addFnAttr(Attribute::OptimizeNone);
  stub->setCallingConv(CallingConv::C);

  IRBuilder<> builder(BasicBlock::Create(stub->getContext(), "entry", stub));
  build(stub, builder);

  return stub;
}

bool valueEscapesLocalBlock(Instruction &value) {
  for (auto const &use : value.uses()) {
    auto inst = cast<Instruction>(&use);
    if (inst->getParent() != inst->getParent() || isa<PHINode>(inst))
      return true;
  }

  return false;
}

void repairSSA(Function &func) {
  auto entryBlock = &func.getEntryBlock();

  std::vector<PHINode *> phis;
  std::vector<Instruction *> regs;
  do {
    phis.clear();
    regs.clear();

    for (auto &block : func) {
      for (auto &inst : block) {
        if (isa<PHINode>(inst)) {
          phis.emplace_back(cast<PHINode>(&inst));
          continue;
        }

        bool canIgnore =
            isa<AllocaInst>(&inst) && inst.getParent() == entryBlock;
        bool needsFixing =
            valueEscapesLocalBlock(inst) || inst.isUsedOutsideOfBlock(&block);
        if (!canIgnore && needsFixing)
          regs.emplace_back(&inst);
      }
    }

    for (auto reg : regs)
      DemoteRegToStack(*reg, entryBlock->getTerminator());
    for (auto phi : phis)
      DemotePHIToStack(phi, entryBlock->getTerminator());
  } while (!regs.empty() || !phis.empty());
}
