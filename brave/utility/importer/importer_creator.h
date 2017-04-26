// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UTILITY_IMPORTER_IMPORTER_CREATOR_H_
#define BRAVE_UTILITY_IMPORTER_IMPORTER_CREATOR_H_

#include "base/memory/ref_counted.h"
#include "chrome/common/importer/importer_type.h"

class Importer;

namespace brave_importer {

// Creates an Importer of the specified |type|.
scoped_refptr<Importer> CreateImporterByType(importer::ImporterType type);

}  // namespace brave_importer

#endif  // BRAVE_UTILITY_IMPORTER_IMPORTER_CREATOR_H_
