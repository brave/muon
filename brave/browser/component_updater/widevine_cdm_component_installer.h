// Copyright (c) 2016 Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_COMPONENT_UPDATER_WIDEVINE_CDM_COMPONENT_INSTALLER_H_
#define BRAVE_BROWSER_COMPONENT_UPDATER_WIDEVINE_CDM_COMPONENT_INSTALLER_H_

#include "base/bind.h"

namespace base {
class FilePath;
}  // namespace base
using ReadyCallback = base::Callback<void(const base::FilePath&)>;

namespace component_updater {
class ComponentUpdateService;
}

namespace brave {
// Our job is to:
// 1) Find what Widevine CDM is installed (if any).
// 2) Register with the component updater to download the latest version when
//    available.
// 3) Copy the Widevine CDM adapter bundled with chrome to the install path.
// 4) Register the Widevine CDM (via the adapter) with Chrome.
// The first part is IO intensive so we do it asynchronously in the file thread.
void RegisterWidevineCdmComponent(
    component_updater::ComponentUpdateService* cus,
    const base::Closure& registered_callback,
    const ReadyCallback& ready_callback);

}  // namespace brave

#endif  // BRAVE_BROWSER_COMPONENT_UPDATER_WIDEVINE_CDM_COMPONENT_INSTALLER_H_
