//===-- Config/Pass/ArithmeticMangler.h - Arithmetic mangler config -------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#ifndef LIMONCELLO_CONFIG_PASS_ARITHMETICMANGLER_H
#define LIMONCELLO_CONFIG_PASS_ARITHMETICMANGLER_H

#include "Limoncello/Config/PassConfig.h"

class ArithmeticManglerConfig : public PassConfig {
public:
  int rounds = 2;
};

template <> struct llvm::yaml::MappingTraits<ArithmeticManglerConfig> {
  static void mapping(IO &io, ArithmeticManglerConfig &config) {
    io.mapOptional("enabled", config.isEnabled);
    io.mapOptional("patterns", config.patterns);

    io.mapOptional("rounds", config.rounds);
  }
};

#endif
