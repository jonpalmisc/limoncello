//===-- Pass/Bloater.cpp - Function-bloating pass -------------------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#include "Limoncello/Pass/Bloater.h"

#include "Limoncello/Config/Config.h"
#include "Limoncello/Support/Function.h"
#include "Limoncello/Support/Module.h"
#include "Limoncello/Support/Random.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ValueMapper.h>

using namespace llvm;

/// Magic value returned from the "opaque true" function.
static constexpr uint64_t BloaterMagic = 0x3636365f4f434d4c;
static constexpr uint64_t BloaterMagicSafeMask = 0xfffffffffffff;

/// Set of predicates that will always result in a truthy value when used to
/// compare a random 32-bit number to the magic value using a comparison in the
/// form of `magic <Predicate> random`.
static constexpr std::array truePredicates = {
    CmpInst::Predicate::ICMP_NE,
    CmpInst::Predicate::ICMP_UGE,
    CmpInst::Predicate::ICMP_UGT,
};

/// Set of predicates to use in comparisons that do not matter; these are used
/// in blocks that are already unreachable (LLVM just can't prove it).
static constexpr std::array fakePredicates = {
    CmpInst::Predicate::ICMP_SGE, CmpInst::Predicate::ICMP_SGT,
    CmpInst::Predicate::ICMP_SLE, CmpInst::Predicate::ICMP_SLT,
    CmpInst::Predicate::ICMP_UGE, CmpInst::Predicate::ICMP_UGT,
    CmpInst::Predicate::ICMP_ULE, CmpInst::Predicate::ICMP_ULT,
};

static constexpr std::array bogusBinaryOps = {
    BinaryOperator::BinaryOps::Add, BinaryOperator::BinaryOps::Sub,
    BinaryOperator::BinaryOps::And, BinaryOperator::BinaryOps::Or,
    BinaryOperator::BinaryOps::Xor,
};

constexpr auto OpaqueGlobalName = "__lmcoBloaterOpaqueGlobal";

Constant *BloaterPass::getOpaqueGlobal(Module &module) {
  return getOrInsertGlobal(
      module, OpaqueGlobalName,
      ConstantInt::get(Type::getInt64Ty(module.getContext()), 0));
}

constexpr auto OpaqueTrueFunctionName = "__lmcoBloaterOpaqueTrue";

Function *BloaterPass::getOpaqueTrueFunction(Module &module) {
  auto callee = module.getOrInsertFunction(
      getModuleSpecificName(module, OpaqueTrueFunctionName),
      FunctionType::get(Type::getInt64Ty(module.getContext()), {}));
  auto func = cast<Function>(callee.getCallee());

  // If we just created the function, it's body needs to be populated.
  if (func->empty()) {
    func->setLinkage(Config::get()->getDefaultLinkage());
    func->addFnAttr(Attribute::OptimizeNone);
    func->addFnAttr(Attribute::NoInline);
    func->setCallingConv(CallingConv::C);

    IRBuilder<> builder(BasicBlock::Create(func->getContext(), "entry", func));
    auto var = builder.CreateAlloca(builder.getInt64Ty());
    builder.CreateStore(builder.getInt64(BloaterMagic), var);
    builder.CreateRet(builder.CreateOr(
        builder.CreateLoad(builder.getInt64Ty(), var),
        builder.CreateLoad(builder.getInt64Ty(), getOpaqueGlobal(module))));
  }

  return func;
}

void BloaterPass::bloatFunction(llvm::Function &func) {
  auto config = Config::get();
  auto &module = *func.getParent();
  auto opaqueGlobal = getOpaqueGlobal(module);

  SmallVector<BasicBlock *> bloatingSet;
  for (auto &block : func) {
    // We don't want the entry block in the bloating set because it creates the
    // potential for the garbage block to branch back to the entry block and
    // the entry block is forbidden from having predecessors.
    if (&block == &func.getEntryBlock())
      continue;

    bloatingSet.emplace_back(&block);
  }

  for (auto block : bloatingSet) {
    auto term = block->getTerminator();
    if (!term)
      continue;
    auto branch = dyn_cast<BranchInst>(term);
    if (!branch || branch->isConditional())
      continue;

    // Bloating every branch matching the conditions above might be a bit too
    // aggressive; roll a die and randomly decide if this branch should be
    // bloated with respect to the configured probability.
    //
    // This comparison looks backwards at first but it makes sense, I promise.
    if (getRandomInt32() % 100 > config->bloater.probability)
      continue;

    // Find where this block is supposed to branch to, then erase the branch.
    auto nextBlock = branch->getSuccessor(0);
    branch->eraseFromParent();

    // Clone the intended destination block in order to create the starting
    // basis for our "garbage block", insert it into the function, then erase
    // the branch to the following block.
    ValueToValueMapTy map;
    auto garbageBlock = CloneBasicBlock(nextBlock, map);
    garbageBlock->insertInto(&func, nextBlock);
    if (garbageBlock->getTerminator())
      garbageBlock->getTerminator()->eraseFromParent();

    // Create a "dispatch block" which really only exists to add complexity to
    // the CFG (this is handy when pairing this pass with the flattener pass)
    // while still serving the purpose of branching to the next block.
    auto dispatchBlock = BasicBlock::Create(func.getContext(), "dispatch");
    dispatchBlock->insertInto(&func, nextBlock);

    // Populate the dispatch block with some garbage code followed by a call
    // to the opaque "always true" function, then use the result to branch to
    // the real block (but retain the garbage block as a possible destination,
    // as far as LLVM is concerned).
    IRBuilder<> dispatchBuilder(dispatchBlock);
    auto opaqueTrue = dispatchBuilder.CreateCmp(
        getRandomItem(truePredicates),
        dispatchBuilder.CreateCall(getOpaqueTrueFunction(module)),
        dispatchBuilder.getInt64(getRandomInt32()));
    dispatchBuilder.CreateCondBr(opaqueTrue, nextBlock, garbageBlock);

    // Add a meaningless store to the opaque global value at the start of the
    // copied block and a meaningless load of the same value at the end.
    IRBuilder<> garbageBuilder(garbageBlock);
    garbageBuilder.SetInsertPoint(garbageBlock, garbageBlock->begin());
    auto bogusArithmetic = garbageBuilder.CreateBinOp(
        getRandomItem(bogusBinaryOps),
        garbageBuilder.CreateLoad(garbageBuilder.getInt64Ty(), opaqueGlobal),
        garbageBuilder.getInt64(getRandomInt32()));
    garbageBuilder.CreateStore(
        garbageBuilder.CreateAnd(bogusArithmetic,
                                 garbageBuilder.getInt64(BloaterMagicSafeMask)),
        opaqueGlobal, /*isVolatile=*/true);
    garbageBuilder.SetInsertPoint(garbageBlock);

    // Insert a conditional branch so that the block has a valid terminator;
    // the destinations are irrelevant (since this block is unreachable in
    // practice) and were picked to optimize for the nastiest decompilation.
    auto fakeCond = garbageBuilder.CreateCmp(
        getRandomItem(fakePredicates),
        garbageBuilder.CreateLoad(garbageBuilder.getInt64Ty(), opaqueGlobal),
        garbageBuilder.getInt64(getRandomInt32()));
    garbageBuilder.CreateCondBr(fakeCond, block, dispatchBlock);

    // Create an unconditional branch to the dispatch block created above.
    IRBuilder<> opaqueBuilder(block);
    opaqueBuilder.CreateBr(dispatchBlock);
  }
}

PreservedAnalyses BloaterPass::run(Module &module, ModuleAnalysisManager &) {
  auto config = Config::get();

  bool changed = false;
  for (auto &func : module.getFunctionList()) {
    if (!config->bloater.shouldRunOnFunction(func))
      continue;

    for (int i = 0; i < config->bloater.rounds; ++i)
      bloatFunction(func);

    repairSSA(func);
    changed |= true;
  }

  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
