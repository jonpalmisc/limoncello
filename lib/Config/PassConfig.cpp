//===-- Config/PassConfig.cpp - Base pass configuration structure ---------===//
//
// Copyright (c) 2023 Jon Palmisciano. All rights reserved.
//
// Use of this source code is governed by the BSD 3-Clause license; a full copy
// of the license can be found in the LICENSE.txt file.
//
//===----------------------------------------------------------------------===//

#include "Limoncello/Config/PassConfig.h"

#include <llvm/Support/Regex.h>

using namespace llvm;

bool PassConfig::shouldRunOnFunction(Function const &function) const {
  if (!isEnabled)
    return false;

  // If no patterns are specified, but the pass is nevertheless enabled, all
  // functions are assumed to be targeted. As soon as one pattern is given,
  // matching behavior will work as expected.
  if (patterns.empty())
    return true;

  for (auto pattern : patterns) {
    // As a cheap hack, let patterns be negated by prepending a tilde; since
    // this isn't actually valid regex, we'll need to note that the pattern
    // should be negated, then erase the tilde.
    bool negate = false;
    if (pattern.starts_with("~")) {
      pattern.erase(0, 1);
      negate = true;
    }

    // TODO: Regexes should be checked for validity when the config is loaded
    // and errors should be reported then, rather than silently failing here.
    Regex regex(pattern);
    if (!regex.isValid())
      continue;

    // If the regex matched, this is either a function we want to include, or a
    // function we want to exclude; in either of these cases, we have an
    // answer. It's important not to simply return the value of the match,
    // since other patterns may match this function even if this one does not.
    if (regex.match(function.getName()))
      return true && !negate;
  }

  return false;
}
