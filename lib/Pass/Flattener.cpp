//===-- Pass/Flattener.cpp - Control flow flattening pass -----------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#include "Limoncello/Pass/Flattener.h"

#include "Limoncello/Config/Config.h"
#include "Limoncello/Support/Function.h"
#include "Limoncello/Support/Random.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/RandomNumberGenerator.h>
#include <llvm/Transforms/Utils/Local.h>

using namespace llvm;

StateMachine::StateMachine(BasicBlock *eb) : m_entryBlock(eb) {
  auto &ctx = m_entryBlock->getContext();
  auto func = m_entryBlock->getParent();

  m_entryBlock->getTerminator()->eraseFromParent();
  IRBuilder<> entryBuilder(m_entryBlock);
  m_stateVar =
      entryBuilder.CreateAlloca(entryBuilder.getInt32Ty(), nullptr, "fsmState");
  m_endBlock = BasicBlock::Create(ctx, "fsmEnd", func, m_entryBlock);
  m_defaultBlock = BasicBlock::Create(ctx, "fsmDefault", func, m_endBlock);
  m_switchBlock = BasicBlock::Create(ctx, "fsmStart", func, m_entryBlock);

  m_defaultBlock->moveBefore(m_endBlock);
  m_switchBlock->moveBefore(m_defaultBlock);
  m_entryBlock->moveBefore(m_switchBlock);

  IRBuilder<> terminalBuilder(m_endBlock);
  terminalBuilder.CreateBr(m_switchBlock);

  IRBuilder<> defaultBuilder(m_defaultBlock);
  defaultBuilder.CreateUnreachable();

  IRBuilder<> dispatchBuilder(m_switchBlock);
  auto switchValue = dispatchBuilder.CreateLoad(dispatchBuilder.getInt32Ty(),
                                                m_stateVar, "state");
  m_switchInst = dispatchBuilder.CreateSwitch(switchValue, m_defaultBlock);
}

bool StateMachine::shouldIgnoreTerminator(Instruction *instruction) {
  if (isa<ReturnInst, UnreachableInst>(instruction))
    return true;

  return false;
}

/// Maximum number of times a case ID should be generated before giving up.
constexpr auto MaxRandomCaseIDAttempts = 3;

void StateMachine::addState(BasicBlock *block) const {
  auto config = Config::get();

  auto getConstantInt32 = [this](uint32_t value) {
    auto &context = m_switchInst->getContext();
    return ConstantInt::get(Type::getInt32Ty(context), value);
  };

  auto stateId = m_switchInst->getNumCases() + 1;
  if (config->flattener.useRandomCaseIds) {
    // We need to make sure that the state ID we just generated are unique,
    // otherwise we'll end up creating two cases with the same ID, which
    // becomes more likely in sufficiently large functions.
    //
    // That said, an infinite loop here would be dangerous and who doesn't like
    // a bit of gambling; will the RNG succeed within the number of allowed
    // attempts or will you have to recompile?
    int attempts = 0;
    while (attempts < MaxRandomCaseIDAttempts) {
      stateId = getRandomInt32() % config->flattener.maxRandomCases;

      auto state = getConstantInt32(stateId);
      if (m_switchInst->findCaseValue(state) != m_switchInst->case_default()) {
        ++attempts;
        continue;
      }

      break;
    }

    // If this is a debug build then we'll get a nice message here, otherwise
    // the verifier pass (or some other part of LLVM) will eventually cry about
    // this; in either case, compiliation won't succeed and this will
    // eventually be communicated to the user.
    if (attempts == MaxRandomCaseIDAttempts) {
      assert(0 && "Failed to generate unique case ID for switch");
    }
  }

  block->moveBefore(m_endBlock);
  m_switchInst->addCase(getConstantInt32(stateId), block);
}

void StateMachine::rewriteBranch(BranchInst *branchInst) {
  auto block = branchInst->getParent();

  if (branchInst->isConditional()) {
    auto trueDest = branchInst->getSuccessor(0);
    auto falseDest = branchInst->getSuccessor(1);

    auto trueDestId = m_switchInst->findCaseDest(trueDest);
    auto falseDestId = m_switchInst->findCaseDest(falseDest);

    IRBuilder<> builder(block);
    auto nextStateId = builder.CreateSelect(branchInst->getCondition(),
                                            trueDestId, falseDestId);
    builder.CreateStore(nextStateId, m_stateVar);
    builder.CreateBr(m_endBlock);
  } else {
    auto dest = branchInst->getSuccessor(0);
    if (dest == m_endBlock)
      return;

    auto destId = m_switchInst->findCaseDest(dest);

    IRBuilder<> builder(block);
    builder.CreateStore(destId, m_stateVar);
    builder.CreateBr(m_endBlock);
  }

  branchInst->eraseFromParent();
}

void StateMachine::rewriteBlock(BasicBlock *block) {
  auto termInst = block->getTerminator();
  if (shouldIgnoreTerminator(termInst))
    return;

  if (isa<ResumeInst, InvokeInst>(termInst))
    assert(1 && "Failed to filter out exception-using function");

  if (auto branchInst = dyn_cast<BranchInst>(termInst))
    rewriteBranch(branchInst);

  assert("Got unhandled block terminator!");
}

void StateMachine::finalize(BasicBlock *firstBlock,
                            SmallVector<BasicBlock *> const &flatteningSet) {
  IRBuilder<> entryBuilder(m_entryBlock);

  auto stateId = m_switchInst->findCaseDest(firstBlock);
  entryBuilder.CreateStore(stateId, m_stateVar);

  for (auto block : flatteningSet)
    rewriteBlock(block);

  entryBuilder.CreateBr(m_switchBlock);
}

SmallVector<BasicBlock *> FlattenerPass::getFlatteningSet(Function &func) {
  auto entryBlock = &func.getEntryBlock();

  SmallVector<BasicBlock *> result;
  for (auto &block : func) {
    if (&block == entryBlock)
      continue;

    result.emplace_back(&block);
  }

  return result;
}

BlockPair FlattenerPass::splitConditionalPart(BasicBlock *block) {
  auto terminator = block->getTerminator();

  Instruction *splitPoint = nullptr;
  if (auto branchInst = dyn_cast<BranchInst>(terminator)) {
    if (branchInst->isConditional()) {
      auto condition = branchInst->getCondition();
      auto conditionInst = dyn_cast<Instruction>(condition);
      assert(conditionInst && "Found conditional branch without condition");

      splitPoint = conditionInst;
    } else {
      splitPoint = terminator;
    }
  } else if (auto switchInst = dyn_cast<SwitchInst>(terminator)) {
    splitPoint = switchInst;
  }

  if (!splitPoint)
    return {block, nullptr};

  auto conditionalPart = block->splitBasicBlock(splitPoint, "conditionalPart");
  return {block, conditionalPart};
}

PreservedAnalyses FlattenerPass::run(Function &func,
                                     FunctionAnalysisManager &) {
  if (!Config::get()->flattener.shouldRunOnFunction(func))
    return PreservedAnalyses::all();

  // TODO: Support C++ exceptions.
  for (auto &block : func) {
    if (block.isLandingPad()) {
      return PreservedAnalyses::all();
    }
  }

  auto [entryBlock, trailingConditionalBlock] =
      splitConditionalPart(&func.getEntryBlock());
  auto flatteningSet = getFlatteningSet(func);
  if (flatteningSet.size() < 2)
    return PreservedAnalyses::none();

  StateMachine stateMachine(entryBlock);
  for (auto block : flatteningSet)
    stateMachine.addState(block);

  stateMachine.finalize(trailingConditionalBlock, flatteningSet);

  repairSSA(func);

  return PreservedAnalyses::none();
}
