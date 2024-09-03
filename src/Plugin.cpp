//===-- Plugin.cpp - Limoncello plugin entry point ------------------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#include "Limoncello/Config/Config.h"
#include "Limoncello/Pass/ArithmeticMangler.h"
#include "Limoncello/Pass/Bloater.h"
#include "Limoncello/Pass/ConstantMangler.h"
#include "Limoncello/Pass/Flattener.h"
#include "Limoncello/Pass/StringObfuscator.h"
#include "Limoncello/Support/Random.h"

#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>

using namespace llvm;

static cl::opt<std::string> configPath("limoncello-config", cl::init(""),
                                       cl::desc("Limoncello config path"));

void registerCallbacks(PassBuilder &pb) {
  auto config = Config::load(configPath);
  if (!config || !config->isValid) {
    errs() << "Limoncello: Invalid configuration; skipping registration...\n";
    return;
  }

  if (config->seed)
    setRandomSeed(config->seed);

  auto addVerifierPass = [&config](ModulePassManager &pm) {
#if 0
    if (config->debug || !NDEBUG)
      pm.addPass(VerifierPass());
#endif
  };

  pb.registerPipelineStartEPCallback([=](ModulePassManager &manager, auto) {
    if (config->stringObfuscator.isEnabled) {
      manager.addPass(StringObfuscatorPass());
      addVerifierPass(manager);
    }
    if (config->bloater.isEnabled) {
      manager.addPass(BloaterPass());
      addVerifierPass(manager);
    }
    if (config->flattener.isEnabled) {
      manager.addPass(createModuleToFunctionPassAdaptor(FlattenerPass()));
      addVerifierPass(manager);
    }
    if (config->constantMangler.isEnabled) {
      manager.addPass(ConstantManglerPass());
      addVerifierPass(manager);
    }
    if (config->arithmeticMangler.isEnabled) {
      manager.addPass(ArithmeticManglerPass());

      // XXX: Do not add a verifier pass after arithmetic mangling; the
      // verifier will whine about using `AlwaysInline` and `OptimizeNone`
      // together on the stub functions created in this pass.
    }
  });
}

PassPluginLibraryInfo getPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "Limoncello", LLVM_VERSION_STRING,
          registerCallbacks};
};

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return getPassPluginInfo();
}
