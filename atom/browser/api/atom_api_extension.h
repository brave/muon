// Copyright (c) 2016 Brave.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_EXTENSION_H_
#define ATOM_BROWSER_API_ATOM_API_EXTENSION_H_

#include <string>
#include "atom/browser/atom_browser_context.h"
#include "atom/common/node_includes.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "native_mate/handle.h"
#include "atom/browser/api/event_emitter.h"

namespace atom {

namespace api {

class WebContents;

class Extension : public mate::EventEmitter<Extension>,
                  public content::NotificationObserver {
 public:
  static mate::Handle<Extension> Create(v8::Isolate* isolate);
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);
  static bool HandleURLOverride(GURL* url,
                                     content::BrowserContext* browser_context);
  static bool HandleURLOverrideReverse(GURL* url,
                                     content::BrowserContext* browser_context);

  static bool IsBackgroundPage(const WebContents* web_contents);
  static v8::Local<v8::Value> TabValue(v8::Isolate* isolate,
                                       WebContents* web_contents);
  static const base::ListValue *
    GetExtensions(const WebContents* web_contents);

 private:
  friend struct base::DefaultSingletonTraits<Extension>;
  explicit Extension(v8::Isolate* isolate);
  ~Extension() override;

  void Observe(
    int type, const content::NotificationSource& source,
    const content::NotificationDetails& details) override;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(Extension);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_EXTENSION_H_
