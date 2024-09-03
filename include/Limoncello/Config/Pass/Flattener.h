//===-- Config/Pass/Flattener.h - Control flow flattener config -----------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#ifndef LIMONCELLO_CONFIG_PASS_FLATTENER_H
#define LIMONCELLO_CONFIG_PASS_FLATTENER_H

#include "Limoncello/Config/PassConfig.h"

class FlattenerConfig : public PassConfig {
public:
  bool useRandomCaseIds = false;
  int maxRandomCases = 768;
};

template <> struct llvm::yaml::MappingTraits<FlattenerConfig> {
  static void mapping(IO &io, FlattenerConfig &config) {
    io.mapOptional("enabled", config.isEnabled);
    io.mapOptional("patterns", config.patterns);

    io.mapOptional("random-case-ids", config.useRandomCaseIds);
    io.mapOptional("max-random-cases", config.maxRandomCases);
  }
};

#endif
