//===-- Config/PassConfig.h - Base pass configuration structure -----------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#ifndef LIMONCELLO_CONFIG_PASSCONFIG_H
#define LIMONCELLO_CONFIG_PASSCONFIG_H

#include <llvm/IR/Function.h>
#include <llvm/Support/YAMLTraits.h>

#include <string>

/// Base pass configuration structure.
class PassConfig {
public:
  bool isEnabled = false;
  std::vector<std::string> patterns{};

  /// Tells whether \p function is matched by \p patterns.
  bool shouldRunOnFunction(llvm::Function const &function) const;
};

#endif
