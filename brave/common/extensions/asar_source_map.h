// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMMON_EXTENSIONS_ASAR_SOURCE_MAP_H_
#define BRAVE_COMMON_EXTENSIONS_ASAR_SOURCE_MAP_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "extensions/renderer/source_map.h"
#include "v8/include/v8.h"

namespace brave {

class AsarSourceMap : public extensions::SourceMap {
 public:
  explicit AsarSourceMap(const std::vector<base::FilePath>& search_paths);
  ~AsarSourceMap() override;

  v8::Local<v8::String> GetSource(v8::Isolate* isolate,
                                 const std::string& name) const override;
  bool Contains(const std::string& name) const override;

 private:
  std::vector<base::FilePath> search_paths_;

  DISALLOW_COPY_AND_ASSIGN(AsarSourceMap);
};

}  // namespace brave

#endif  // BRAVE_COMMON_EXTENSIONS_ASAR_SOURCE_MAP_H_
