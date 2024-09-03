//===-- Config/Pass/ConstantMangler.h - Constant mangler config -----------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#ifndef LIMONCELLO_CONFIG_PASS_CONSTANTMANGLER_H
#define LIMONCELLO_CONFIG_PASS_CONSTANTMANGLER_H

#include "Limoncello/Config/PassConfig.h"

class ConstantManglerConfig : public PassConfig {};

template <> struct llvm::yaml::MappingTraits<ConstantManglerConfig> {
  static void mapping(IO &io, ConstantManglerConfig &config) {
    io.mapOptional("enabled", config.isEnabled);
    io.mapOptional("patterns", config.patterns);
  }
};

#endif
