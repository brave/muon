// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_ATOM_COMMAND_LINE_H_
#define ATOM_COMMON_ATOM_COMMAND_LINE_H_

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/macros.h"
#include "build/build_config.h"

namespace atom {

// Singleton to remember the original "argc" and "argv".
class AtomCommandLine {
 public:
  using StringVector = std::vector<std::string>;

#if defined(OS_WIN)
  static void InitW(int argc, const wchar_t* const* argv);
#else
  static void Init(int argc, const char* const* argv);
#endif
  static const StringVector& argv_utf8() { return argv_utf8_; }
  static const base::CommandLine::StringVector& argv() { return argv_; }

 private:
  static base::CommandLine::StringVector argv_;
  static StringVector argv_utf8_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(AtomCommandLine);
};

}  // namespace atom

#endif  // ATOM_COMMON_ATOM_COMMAND_LINE_H_
