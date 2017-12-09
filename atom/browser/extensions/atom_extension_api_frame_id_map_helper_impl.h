// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_API_FRAME_ID_MAP_HELPER_IMPL_H_
#define ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_API_FRAME_ID_MAP_HELPER_IMPL_H_

#include "atom/browser/extensions/atom_extension_api_frame_id_map.h"
#include "base/macros.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace extensions {

class AtomExtensionApiFrameIdMapHelperImpl
    : public AtomExtensionApiFrameIdMapHelper,
      public content::NotificationObserver {
 public:
  explicit AtomExtensionApiFrameIdMapHelperImpl(
      AtomExtensionApiFrameIdMap* owner);
  ~AtomExtensionApiFrameIdMapHelperImpl() override;

 private:
  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  AtomExtensionApiFrameIdMap* owner_;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionApiFrameIdMapHelperImpl);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_API_FRAME_ID_MAP_HELPER_IMPL_H_
