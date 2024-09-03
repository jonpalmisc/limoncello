//===-- Support/Random.cpp - Random number generation shorthand(s) --------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#include "Limoncello/Support/Random.h"

#include <random>

static std::mt19937 g_mt{std::random_device()()};
static std::uniform_int_distribution<uint64_t>
    g_rng(0, std::numeric_limits<uint64_t>::max());

void setRandomSeed(unsigned seed) { g_mt.seed(seed); }

uint8_t getRandomInt8() { return static_cast<uint8_t>(g_rng(g_mt)); }
uint32_t getRandomInt32() { return static_cast<uint32_t>(g_rng(g_mt)); }
uint64_t getRandomInt64() { return static_cast<uint64_t>(g_rng(g_mt)); }
