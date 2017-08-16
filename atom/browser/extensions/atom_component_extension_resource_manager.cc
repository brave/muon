// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_component_extension_resource_manager.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "brave/grit/brave_resources.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/grit/component_extension_resources_map.h"

namespace extensions {

AtomComponentExtensionResourceManager::
AtomComponentExtensionResourceManager() {
  static const GritResourceMap kExtraComponentExtensionResources[] = {
    {"cryptotoken/manifest.json", IDR_CRYPTOTOKEN_MANIFEST},
  };

  AddComponentResourceEntries(
      kComponentExtensionResources,
      kComponentExtensionResourcesSize);
  AddComponentResourceEntries(
      kExtraComponentExtensionResources,
      arraysize(kExtraComponentExtensionResources));
}

AtomComponentExtensionResourceManager::
~AtomComponentExtensionResourceManager() {}

bool AtomComponentExtensionResourceManager::IsComponentExtensionResource(
    const base::FilePath& extension_path,
    const base::FilePath& resource_path,
    int* resource_id) const {
  base::FilePath directory_path = extension_path;
  base::FilePath resources_dir;
  base::FilePath relative_path;

  if (!PathService::Get(chrome::DIR_RESOURCES, &resources_dir) ||
      !resources_dir.AppendRelativePath(directory_path, &relative_path)) {
    return false;
  }
  relative_path = relative_path.Append(resource_path);
  relative_path = relative_path.NormalizePathSeparators();

  std::map<base::FilePath, int>::const_iterator entry =
      path_to_resource_id_.find(relative_path);
  if (entry != path_to_resource_id_.end())
    *resource_id = entry->second;

  return entry != path_to_resource_id_.end();
}

void AtomComponentExtensionResourceManager::AddComponentResourceEntries(
    const GritResourceMap* entries,
    size_t size) {
  for (size_t i = 0; i < size; ++i) {
    base::FilePath resource_path = base::FilePath().AppendASCII(
        entries[i].name);
    resource_path = resource_path.NormalizePathSeparators();

    DCHECK(path_to_resource_id_.find(resource_path) ==
        path_to_resource_id_.end());
    path_to_resource_id_[resource_path] = entries[i].value;
  }
}

}  // namespace extensions
