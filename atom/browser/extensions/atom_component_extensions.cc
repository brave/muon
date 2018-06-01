// Copyright 2018 Brave authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_component_extensions.h"

#include <map>

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "brave/grit/brave_resources.h"
#include "chrome/common/chrome_paths.h"

static const std::map<base::FilePath, int> componentExtensionManifests = {
    {base::FilePath(FILE_PATH_LITERAL("cryptotoken")),
     IDR_CRYPTOTOKEN_MANIFEST},
};

bool IsComponentExtension(const base::FilePath& extension_path,
                          int* resource_id) {
  base::FilePath directory_path = extension_path;
  base::FilePath resources_dir;
  base::FilePath relative_path;

  if (!base::PathService::Get(chrome::DIR_RESOURCES, &resources_dir) ||
      !resources_dir.AppendRelativePath(directory_path, &relative_path)) {
    return false;
  }
  relative_path = relative_path.NormalizePathSeparators();

  std::map<base::FilePath, int>::const_iterator entry =
      componentExtensionManifests.find(relative_path);

  if (entry != componentExtensionManifests.end())
    *resource_id = entry->second;

  return entry != componentExtensionManifests.end();
}
