// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/common/extensions/asar_source_map.h"

#include "atom/common/asar/asar_util.h"
#include "base/files/file_util.h"
#include "base/strings/string_split.h"
#include "gin/converter.h"

namespace brave {

AsarSourceMap::AsarSourceMap(
    const std::vector<base::FilePath>& search_paths)
    : search_paths_(search_paths) {
}

AsarSourceMap::~AsarSourceMap() {
}

v8::Local<v8::Value> AsarSourceMap::GetSource(
    v8::Isolate* isolate,
    const std::string& name) const {
  std::vector<std::string> components = base::SplitString(
      name, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  base::FilePath path;
  for (size_t i = 0; i < components.size(); ++i) {
    path = path.AppendASCII(components[i]);
  }
  path = path.AddExtension(FILE_PATH_LITERAL("js"));

  std::string source;
  for (size_t i = 0; i < search_paths_.size(); ++i) {
    base::FilePath archive;
    base::FilePath relative;
    if (asar::GetAsarArchivePath(search_paths_[i], &archive, &relative)) {
      if (!asar::ReadFileToString(search_paths_[i], &source)) {
        continue;
      }
    } else if (!ReadFileToString(search_paths_[i].Append(path), &source)) {
      continue;
    }
    return gin::StringToV8(isolate, source);
  }

  NOTREACHED() << "No module is registered with name \"" << name << "\"";
  return v8::Undefined(isolate);
}

bool AsarSourceMap::Contains(const std::string& name) const {
  // TODO(bridiver) - fix this
  return true;
}

}  // namespace brave
