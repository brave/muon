// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_extension_api_frame_id_map.h"

#include "atom/browser/extensions/atom_extension_api_frame_id_map_helper_impl.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace extensions {

namespace {

base::LazyInstance<AtomExtensionApiFrameIdMap>::Leaky g_map_instance =
    LAZY_INSTANCE_INITIALIZER;

bool IsFrameRoutingIdValid(int frame_routing_id) {
  return frame_routing_id > -1;
}

}  // namespace

const int AtomExtensionApiFrameIdMap::kInvalidFrameId = -1;

AtomExtensionApiFrameIdMap::FrameData::FrameData()
    : tab_id(-1),
      window_id(-1) {}

AtomExtensionApiFrameIdMap::FrameData::FrameData(int tab_id,
                                                 int window_id)
    : tab_id(tab_id),
      window_id(window_id) {}

AtomExtensionApiFrameIdMap::RenderFrameIdKey::RenderFrameIdKey()
    : frame_tree_node_id(kInvalidFrameId) {}

AtomExtensionApiFrameIdMap::RenderFrameIdKey::RenderFrameIdKey(
    int frame_tree_node_id)
    : frame_tree_node_id(frame_tree_node_id) {}

bool AtomExtensionApiFrameIdMap::RenderFrameIdKey::operator<(
    const RenderFrameIdKey& other) const {
  return frame_tree_node_id < other.frame_tree_node_id;
}

bool AtomExtensionApiFrameIdMap::RenderFrameIdKey::operator==(
    const RenderFrameIdKey& other) const {
  return frame_tree_node_id == other.frame_tree_node_id;
}

AtomExtensionApiFrameIdMap::AtomExtensionApiFrameIdMap() :
  helper_(new AtomExtensionApiFrameIdMapHelperImpl(this)) {}

AtomExtensionApiFrameIdMap::~AtomExtensionApiFrameIdMap() {}

// static
AtomExtensionApiFrameIdMap* AtomExtensionApiFrameIdMap::Get() {
  return g_map_instance.Pointer();
}

AtomExtensionApiFrameIdMap::FrameData AtomExtensionApiFrameIdMap::KeyToValue(
    const RenderFrameIdKey& key) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  int tab_id = -1;
  int window_id = -1;
  int frame_id = kInvalidFrameId;

  auto web_contents =
      content::WebContents::FromFrameTreeNodeId(key.frame_tree_node_id);
  if (web_contents) {
    tab_id = SessionTabHelper::IdForTab(web_contents);
    window_id = SessionTabHelper::IdForWindowContainingTab(web_contents);
  }

  return FrameData(tab_id, window_id);
}

AtomExtensionApiFrameIdMap::FrameData
AtomExtensionApiFrameIdMap::LookupFrameDataOnUI(
    const RenderFrameIdKey& key) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  bool lookup_successful = false;
  FrameData data;
  FrameDataMap::const_iterator frame_id_iter = frame_data_map_.find(key);
  if (frame_id_iter != frame_data_map_.end()) {
    lookup_successful = true;
    data = frame_id_iter->second;
  } else {
    data = KeyToValue(key);
    // Don't save invalid values in the map.
    if (data.tab_id != -1) {
      lookup_successful = true;
      auto kvpair = FrameDataMap::value_type(key, data);
      base::AutoLock lock(frame_data_map_lock_);
      frame_data_map_.insert(kvpair);
    }
  }

  return data;
}

bool AtomExtensionApiFrameIdMap::GetCachedFrameDataOnIO(
                                                    int frame_tree_node_id,
                                                    FrameData* frame_data_out) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  if (!IsFrameRoutingIdValid(frame_tree_node_id))
    return false;

  bool found = false;
  {
    base::AutoLock lock(frame_data_map_lock_);
    FrameDataMap::const_iterator frame_id_iter = frame_data_map_.find(
        RenderFrameIdKey(frame_tree_node_id));
    if (frame_id_iter != frame_data_map_.end()) {
      // This is very likely to happen because CacheFrameData() is called as
      // soon as the frame is created.
      *frame_data_out = frame_id_iter->second;
      found = true;
    }
  }

  return found;
}

void AtomExtensionApiFrameIdMap::CacheFrameData(content::RenderFrameHost* rfh) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const RenderFrameIdKey key(rfh->GetFrameTreeNodeId());
  CacheFrameData(key);
  DCHECK(frame_data_map_.find(key) != frame_data_map_.end());
}

void AtomExtensionApiFrameIdMap::CacheFrameData(const RenderFrameIdKey& key) {
  LookupFrameDataOnUI(key);
}

void AtomExtensionApiFrameIdMap::RemoveFrameData(
    content::RenderFrameHost* rfh) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(rfh);

  const RenderFrameIdKey key(rfh->GetFrameTreeNodeId());
  RemoveFrameData(key);
}

void AtomExtensionApiFrameIdMap::UpdateTabAndWindowId(
    int tab_id,
    int window_id,
    content::RenderFrameHost* rfh) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(rfh);
  const RenderFrameIdKey key(rfh->GetFrameTreeNodeId());
  base::AutoLock lock(frame_data_map_lock_);
  FrameDataMap::iterator iter = frame_data_map_.find(key);
  if (iter != frame_data_map_.end()) {
    iter->second.tab_id = tab_id;
    iter->second.window_id = window_id;
  } else {
    frame_data_map_[key] =
        FrameData(tab_id, window_id);
  }
}

void AtomExtensionApiFrameIdMap::RemoveFrameData(const RenderFrameIdKey& key) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::AutoLock lock(frame_data_map_lock_);
  frame_data_map_.erase(key);
}

}  // namespace extensions
