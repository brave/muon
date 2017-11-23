// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_API_FRAME_ID_MAP_H_
#define ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_API_FRAME_ID_MAP_H_

#include <map>
#include <memory>

#include "base/callback.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace extensions {

class AtomExtensionApiFrameIdMapHelper {
 public:
  virtual ~AtomExtensionApiFrameIdMapHelper() {}
};

class AtomExtensionApiFrameIdMap {
 public:
  struct FrameData {
    FrameData();
    FrameData(int tab_id, int window_id);
    int tab_id;
    int window_id;
  };

  // An invalid extension API frame ID.
  static const int kInvalidFrameId;

  static AtomExtensionApiFrameIdMap* Get();

  bool GetCachedFrameDataOnIO(int frame_tree_node_id,
                              FrameData* frame_data_out);
  void CacheFrameData(content::RenderFrameHost* rfh);
  void RemoveFrameData(content::RenderFrameHost* rfh);
  void UpdateTabAndWindowId(int tab_id,
                            int window_id,
                            content::RenderFrameHost* rfh);

 protected:
  friend struct base::LazyInstanceTraitsBase<AtomExtensionApiFrameIdMap>;

  // A set of identifiers that uniquely identifies a RenderFrame.
  struct RenderFrameIdKey {
    RenderFrameIdKey();
    explicit RenderFrameIdKey(int frame_tree_node_id);

    int frame_tree_node_id;

    bool operator<(const RenderFrameIdKey& other) const;
    bool operator==(const RenderFrameIdKey& other) const;
  };

  using FrameDataMap = std::map<RenderFrameIdKey, FrameData>;

  AtomExtensionApiFrameIdMap();
  virtual ~AtomExtensionApiFrameIdMap();

  virtual FrameData KeyToValue(const RenderFrameIdKey& key) const;
  FrameData LookupFrameDataOnUI(const RenderFrameIdKey& key);
  void CacheFrameData(const RenderFrameIdKey& key);
  void RemoveFrameData(const RenderFrameIdKey& key);

  std::unique_ptr<AtomExtensionApiFrameIdMapHelper> helper_;
  FrameDataMap frame_data_map_;
  base::Lock frame_data_map_lock_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomExtensionApiFrameIdMap);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_API_FRAME_ID_MAP_H_
