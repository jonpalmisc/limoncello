//===-- Config/Config.h - Top-level configuration structure ---------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#ifndef LIMONCELLO_CONFIG_CONFIG_H
#define LIMONCELLO_CONFIG_CONFIG_H

#include "Limoncello/Config/Pass/ArithmeticMangler.h"
#include "Limoncello/Config/Pass/Bloater.h"
#include "Limoncello/Config/Pass/ConstantMangler.h"
#include "Limoncello/Config/Pass/Flattener.h"
#include "Limoncello/Config/Pass/StringObfuscator.h"

/// Top-level obfuscator configuration structure.
class Config {
  /// Create the default configuration.
  Config();

  /// Load a config from the file at \p path.
  ///
  /// Will return an invalid config if \p path is a file that does not exist,
  /// or if the file at \p path fails to parse as configuration YAML.
  explicit Config(std::string const &path);

  /// Update the current config by loading values from YAML string \p yaml.
  void loadFromYAML(std::string const &yaml);

public:
  bool isValid;

  /// Controls obfuscator output, including presence of symbol names, etc.
  bool debug;

  /// Seed for the obfuscator's RNG; random if not provided.
  unsigned seed;

  ArithmeticManglerConfig arithmeticMangler;
  BloaterConfig bloater;
  ConstantManglerConfig constantMangler;
  FlattenerConfig flattener;
  StringObfuscatorConfig stringObfuscator;

  /// Load the global config from \p path.
  static Config *load(std::string path = "");

  /// Get the global configuration.
  static Config *get();

  /// Get the default linkage for Limoncello-related symbols and functions.
  llvm::GlobalValue::LinkageTypes getDefaultLinkage() const {
    return debug ? llvm::GlobalValue::InternalLinkage
                 : llvm::GlobalValue::PrivateLinkage;
  }
};

template <> struct llvm::yaml::MappingTraits<Config> {
  static void mapping(llvm::yaml::IO &io, Config &config) {
    io.mapOptional("debug", config.debug);
    io.mapOptional("seed", config.seed);

    io.mapOptional("arithmetic-mangler", config.arithmeticMangler);
    io.mapOptional("bloater", config.bloater);
    io.mapOptional("constant-mangler", config.constantMangler);
    io.mapOptional("flattener", config.flattener);
    io.mapOptional("string-obfuscator", config.stringObfuscator);
  }
};

#endif
