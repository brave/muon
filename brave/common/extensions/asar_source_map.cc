// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/common/extensions/asar_source_map.h"

#include "atom/common/asar/asar_util.h"
#include "base/callback.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/strings/string_split.h"
#include "gin/converter.h"

namespace brave {

namespace {

static const char commonjs[] = "muon/module_system/commonjs";

bool IsAsarPath(const base::FilePath& path) {
  base::FilePath archive;
  base::FilePath relative;
  return asar::GetAsarArchivePath(path, &archive, &relative);
}

bool ReadFromPath(
    base::Callback<bool(const base::FilePath& path, std::string* contents)>,
    const base::FilePath& file,
    const base::FilePath& path,
    std::string* source) {
  base::FilePath file_path = path.Append(file);
  if (!file_path.MatchesExtension(FILE_PATH_LITERAL(".js")))
    file_path = file_path.AddExtension(FILE_PATH_LITERAL("js"));

  base::FilePath module_path1 = path
      .Append(file)
      .Append(FILE_PATH_LITERAL("index"))
      .AddExtension(FILE_PATH_LITERAL("js"));

  base::FilePath module_path2 = path
      .Append(file)
      .Append(file)
      .AddExtension(FILE_PATH_LITERAL("js"));

  return asar::ReadFileToString(file_path, source) ||
      asar::ReadFileToString(module_path1, source) ||
      asar::ReadFileToString(module_path2, source);
}

bool ReadFromSearchPaths(const std::vector<base::FilePath>& search_paths,
                        const base::FilePath& file_path,
                        std::string* source) {
  for (size_t i = 0; i < search_paths.size(); ++i) {
    if (IsAsarPath(search_paths[i])) {
      if (!ReadFromPath(base::Bind(&asar::ReadFileToString),
          file_path, search_paths[i], source))
        continue;
    } else {
      if (!ReadFromPath(base::Bind(&base::ReadFileToString),
          file_path, search_paths[i], source))
        continue;
    }
    return true;
  }
  return false;
}

const base::FilePath GetFilePath(const std::string& name) {
  std::vector<std::string> components = base::SplitString(
      name, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  std::vector<base::FilePath::StringType> path_components;

  // normalize the path
  for (size_t i = 0; i < components.size(); ++i) {
    base::FilePath::StringType component(components[i].begin(), components[i].end());

    if (component == base::FilePath::kCurrentDirectory)
      continue;

    if (component == base::FilePath::kParentDirectory) {
      path_components.pop_back();
    } else {
      path_components.push_back(component);
    }
  }

  base::FilePath path;
  for (size_t i = 0; i < path_components.size(); ++i) {
    path = path.Append(path_components[i]);
  }
  return path;
}

}  // namespace

AsarSourceMap::AsarSourceMap(
    const std::vector<base::FilePath>& search_paths)
    : search_paths_(search_paths) {
}

AsarSourceMap::~AsarSourceMap() {
}

v8::Local<v8::String> AsarSourceMap::GetSource(
    v8::Isolate* isolate,
    const std::string& name) const {
  std::string source;
  if (ReadFromSearchPaths(search_paths_, GetFilePath(name), &source)) {
    if (name != commonjs) {
      source =
          "const fn = function (require, module, console) { " + source + " };"
          "require('" +
            commonjs +
          "').require(fn, exports, '" +
          GetFilePath(name).AsUTF8Unsafe() +
          "', this);";
    }

    return gin::StringToV8(isolate, source);
  }

  NOTREACHED() << "No module is registered with name \"" << name << "\"";
  return v8::Local<v8::String>();
}

bool AsarSourceMap::Contains(const std::string& name) const {
  std::string source;
  return ReadFromSearchPaths(search_paths_, GetFilePath(name), &source);
}

}  // namespace brave
