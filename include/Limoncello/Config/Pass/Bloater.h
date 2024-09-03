//===-- Config/Pass/Bloater.h - Bloater config ----------------------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#ifndef LIMONCELLO_CONFIG_PASS_BLOATER_H
#define LIMONCELLO_CONFIG_PASS_BLOATER_H

#include "Limoncello/Config/PassConfig.h"

class BloaterConfig : public PassConfig {
public:
  int rounds = 1;
  int probability = 50;
};

template <> struct llvm::yaml::MappingTraits<BloaterConfig> {
  static void mapping(IO &io, BloaterConfig &config) {
    io.mapOptional("enabled", config.isEnabled);
    io.mapOptional("patterns", config.patterns);

    io.mapOptional("rounds", config.rounds);
    io.mapOptional("probability", config.probability);
  }
};

#endif
