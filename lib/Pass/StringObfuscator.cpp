//===-- Pass/StringObfuscator.cpp - String obfuscation pass ---------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#include "Limoncello/Pass/StringObfuscator.h"

#include "Limoncello/Support/Function.h"
#include "Limoncello/Support/Module.h"
#include "Limoncello/Support/Random.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/RandomNumberGenerator.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>

#include "StringObfuscatorBitcode.h"

constexpr auto FunctionNameDeobfuscate = "__lmcoStringDeobfuscateXOR";
constexpr auto FunctionNameDeobfuscateAll = "__lmcoStringDeobfuscateAll";

using namespace llvm;

Constant *StringObfuscatorPass::createObfuscatedString(LLVMContext &context,
                                                       StringRef content,
                                                       uint8_t key) {
  std::string result(content.size(), 0);
  std::transform(content.begin(), content.end() - 1, result.begin(),
                 [key](auto c) { return c ^ key; });

  return ConstantDataArray::getString(context, StringRef(result), false);
}

std::vector<ObfuscatedStringRecord>
StringObfuscatorPass::obfuscateStrings(Module &module) {
  std::vector<ObfuscatedStringRecord> modifiedGlobals;
  for (auto &var : module.globals()) {
    // Can't operate on uninitialized or imported globals.
    if (!var.hasInitializer() || var.hasExternalLinkage())
      continue;

    // Initializer must be both a constant data array and a string.
    auto initializer = var.getInitializer();
    auto data = dyn_cast<ConstantDataArray>(initializer);
    if (!data || !data->isString())
      continue;

    auto content = data->getAsString();
    uint8_t xorKey = getRandomInt8();

    var.setInitializer(
        createObfuscatedString(module.getContext(), content, xorKey));
    var.setConstant(false);

    // The null byte is included in `content.size()` and should not be touched
    // when deobfuscating strings.
    modifiedGlobals.emplace_back(&var, content.size() - 1, xorKey);
  }

  return modifiedGlobals;
}

Function *StringObfuscatorPass::getDeobfuscateFunction(Module &module) {
  SMDiagnostic unusedError;

  auto runtimeModule =
      parseIR(getDeobfuscateBitcode(), unusedError, module.getContext());
  assert(runtimeModule && "Failed to parse string obfuscator runtime IR!");

  // TODO: Ideally the name of the runtime function doesn't live here as a
  // magic string and is defined elsewhere.
  auto deobfuscateFn = runtimeModule->getFunction("deobfuscateXOR");
  assert(deobfuscateFn && "String deobfuscation function missing in runtime!");

  return localizeFunction(
      module, *deobfuscateFn,
      getModuleSpecificName(module, FunctionNameDeobfuscate));
}

Function *StringObfuscatorPass::createDeobfuscateAllFunction(
    Module &module, std::vector<ObfuscatedStringRecord> const &records) {
  auto &context = module.getContext();

  auto callee = module.getOrInsertFunction(
      getModuleSpecificName(module, FunctionNameDeobfuscateAll),
      Type::getVoidTy(context));
  auto result = cast<Function>(callee.getCallee());

  auto deobfuscateFn = getDeobfuscateFunction(module);

  // Build a huge block with a call to the deobfuscate function for every
  // single obfuscated string in the binary.
  auto body = BasicBlock::Create(context, "", result);
  IRBuilder<> builder(body);
  for (auto string : records) {
    builder.CreateCall(deobfuscateFn, {/*pointer=*/string.handle,
                                       /*size=*/builder.getInt64(string.size),
                                       /*key=*/builder.getInt8(string.key)});
  }
  builder.CreateRetVoid();

  // TODO: This used to be done to prevent the optimizer from deleting these
  // function (calls) from the module. Now, commenting out these lines does not
  // break the string obfuscation, but the routines do tend to get inlined
  // which is debatably desirable. This should be looked into further.
  result->addFnAttr(Attribute::get(context, Attribute::NoInline));
  result->addFnAttr(Attribute::get(context, Attribute::OptimizeNone));

  return result;
}

PreservedAnalyses StringObfuscatorPass::run(Module &module,
                                            ModuleAnalysisManager &) {
  auto obfuscatedStrings = obfuscateStrings(module);
  auto deobfuscateAllFn =
      createDeobfuscateAllFunction(module, obfuscatedStrings);

  if (auto mainFn = module.getFunction("main")) {
    auto &entryBlock = mainFn->getEntryBlock();
    auto callBlock = BasicBlock::Create(mainFn->getContext(), "",
                                        entryBlock.getParent(), &entryBlock);

    // If this is an executable, it's easy enough (and less suspicious) to shim
    // the deobfuscation routine into the start of `main`.
    IRBuilder<> builder(callBlock);
    builder.CreateCall(deobfuscateAllFn);
    builder.CreateBr(&entryBlock);
  } else {
    // However, if `main` isn't present (e.g. this is a library), we can add
    // the "deobfuscate all strings" function as a global constructor to ensure
    // it is run. Setting the priority to 0 here is important as there is a
    // potential for a data hazard if other global constructors depend on
    // strings which have yet to be deobfuscated.
    appendToGlobalCtors(module, deobfuscateAllFn, /*priority=*/0);
  }

  return PreservedAnalyses::none();
}
