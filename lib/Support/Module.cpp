//===-- Support/Module.cpp - Module-related helpers -----------------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#include "Limoncello/Support/Module.h"

#include "Limoncello/Config/Config.h"

#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/MD5.h>

using namespace llvm;

std::string getModuleSpecificName(Module &module, StringRef name) {
  auto salt = arrayRefFromStringRef(module.getModuleIdentifier());
  return name.str() + "$" +
         toHex(MD5::hash(salt), /*lowerCase=*/true).substr(0, 16);
}

Constant *getOrInsertGlobal(Module &module, StringRef name, Constant *value) {
  return module.getOrInsertGlobal(name, value->getType(), [&] {
    return new GlobalVariable(module, value->getType(), /*isConstant=*/false,
                              Config::get()->getDefaultLinkage(), value, name);
  });
}