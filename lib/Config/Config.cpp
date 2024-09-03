//===-- Config/Config.cpp - Top-level configuration structure -------------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#include "Limoncello/Config/Config.h"

#include <fstream>
#include <sstream>

using namespace llvm;

Config::Config() : isValid(true), debug(false), seed(0) {}

Config::Config(std::string const &path) : Config() {
  std::string yaml;
  try {
    std::ifstream inputFile(path);
    yaml = std::string(std::istreambuf_iterator<char>(inputFile),
                       std::istreambuf_iterator<char>());
  } catch (...) {
    isValid = false;
    return;
  }

  loadFromYAML(yaml);
}

void Config::loadFromYAML(std::string const &yaml) {
  yaml::Input yamlParser(yaml);
  yamlParser >> *this;

  if (yamlParser.error())
    isValid = false;
}

static Config *g_config = nullptr;

Config *Config::load(std::string path) {
  g_config = path.empty() ? new Config : new Config(path);
  return g_config;
}

Config *Config::get() {
  if (!g_config)
    g_config = new Config;

  return g_config;
}
