// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_CONTENT_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_CONTENT_CONVERTER_H_

#include <utility>

#include "content/public/browser/permission_type.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/restore_type.h"
#include "content/public/common/menu_item.h"
#include "content/public/common/page_type.h"
#include "content/public/common/stop_find_action.h"
#include "native_mate/converter.h"
#include "third_party/blink/public/platform/modules/permissions/permission_status.mojom.h"
#include "ui/base/page_transition_types.h"

namespace content {
struct ContextMenuParams;
class WebContents;
class NavigationController;
class NavigationEntry;
class NavigationHandle;
}

using ContextMenuParamsWithWebContents =
    std::pair<content::ContextMenuParams, content::WebContents*>;

namespace mate {

template<>
struct Converter<content::MenuItem::Type> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const content::MenuItem::Type& val);
};

template<>
struct Converter<ContextMenuParamsWithWebContents> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const ContextMenuParamsWithWebContents& val);
};

template<>
struct Converter<blink::mojom::PermissionStatus> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     blink::mojom::PermissionStatus* out);
};

template<>
struct Converter<content::PermissionType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const content::PermissionType& val);
};

template<>
struct Converter<content::StopFindAction> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     content::StopFindAction* out);
};

template<>
struct Converter<content::WebContents*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   content::WebContents* val);
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     content::WebContents** out);
};

template<>
struct Converter<content::NavigationController*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   content::NavigationController* val);
};

template<>
struct Converter<content::NavigationEntry*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   content::NavigationEntry* val);
};

template<>
struct Converter<content::NavigationHandle*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   content::NavigationHandle* val);
};

template<>
struct Converter<content::PageType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const content::PageType& val);
};

template<>
struct Converter<content::ReloadType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const content::ReloadType& val);
};

template<>
struct Converter<content::RestoreType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const content::RestoreType& val);
};

template<>
struct Converter<ui::PageTransition> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const ui::PageTransition& val);
};
}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_CONTENT_CONVERTER_H_
