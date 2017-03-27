// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/atom_command_line.h"

#include "base/command_line.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "node/deps/uv/include/uv.h"

using base::CommandLine;

namespace atom {

CommandLine::StringVector AtomCommandLine::argv_;
AtomCommandLine::StringVector AtomCommandLine::argv_utf8_;

// static
#if defined(OS_WIN)
void AtomCommandLine::InitW(int argc, const wchar_t* const* argv) {
#else
void AtomCommandLine::Init(int argc, const char* const* argv) {
#endif
  argv_.clear();
  argv_utf8_.clear();

  for (int i = 0; i < argc; ++i) {
#if defined(OS_WIN)
    argv_utf8_.push_back(base::SysWideToUTF8(argv[i]));
#else
    argv_utf8_.push_back(CommandLine::StringType(argv[i]));
#endif
    argv_.push_back(CommandLine::StringType(argv[i]));
  }
}

}  // namespace atom
