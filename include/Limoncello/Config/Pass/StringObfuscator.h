//===-- Config/Pass/StringObfuscator.h - String obfuscator config ---------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#ifndef LIMONCELLO_CONFIG_PASS_STRINGOBFUSCATOR_H
#define LIMONCELLO_CONFIG_PASS_STRINGOBFUSCATOR_H

#include "Limoncello/Config/PassConfig.h"

class StringObfuscatorConfig : public PassConfig {};

template <> struct llvm::yaml::MappingTraits<StringObfuscatorConfig> {
  static void mapping(IO &io, StringObfuscatorConfig &config) {
    io.mapOptional("enabled", config.isEnabled);
    io.mapOptional("patterns", config.patterns);
  }
};

#endif
