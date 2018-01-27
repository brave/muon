// Copyright 2018 Brave authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_ATOM_COMPONENT_EXTENSIONS_H_
#define ATOM_BROWSER_EXTENSIONS_ATOM_COMPONENT_EXTENSIONS_H_

#include "base/files/file_path.h"

bool IsComponentExtension(const base::FilePath& extension_path,
                          int* resource_id);

#endif  // ATOM_BROWSER_EXTENSIONS_ATOM_COMPONENT_EXTENSIONS_H_
