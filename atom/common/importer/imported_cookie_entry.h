// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_IMPORTER_IMPORTED_COOKIE_ENTRY_H_
#define ATOM_COMMON_IMPORTER_IMPORTED_COOKIE_ENTRY_H_

#include "base/strings/string16.h"
#include "base/time/time.h"

struct ImportedCookieEntry {
  ImportedCookieEntry();
  ~ImportedCookieEntry();

  base::string16 domain;

  base::string16 name;

  base::string16 value;

  base::string16 host;

  base::string16 path;

  base::Time expiry_date;

  bool secure;

  bool httponly;
};

#endif  // ATOM_COMMON_IMPORTER_IMPORTED_COOKIE_ENTRY_H_
