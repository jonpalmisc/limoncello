//===-- Support/Random.h - Random number generation shorthand(s) ----------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#ifndef LIMONCELLO_SUPPORT_RANDOM_H
#define LIMONCELLO_SUPPORT_RANDOM_H

#include <cstdint>

/// Seed the backing random number generator.
void setRandomSeed(unsigned seed);

/// Get a random 8-bit value.
uint8_t getRandomInt8();

/// Get a random 32-bit value.
uint32_t getRandomInt32();

/// Get a random 64-bit value.
uint64_t getRandomInt64();

/// Get a random item from a container.
template <typename ContainerTy>
typename ContainerTy::value_type getRandomItem(ContainerTy container) {
  return container[getRandomInt64() % container.size()];
}

#endif
