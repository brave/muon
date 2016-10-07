// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_EXTENSION_H_
#define ATOM_BROWSER_API_ATOM_API_EXTENSION_H_

#include <string>
#include "atom/browser/api/trackable_object.h"
#include "atom/browser/atom_browser_context.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/manifest.h"
#include "native_mate/handle.h"

namespace base {
class FilePath;
}

namespace content {
class BrowserContext;
}

namespace extensions {
class Extension;
}

namespace mate {
class Dictionary;
}

namespace atom {

namespace api {

class WebContents;

class Extension : public mate::TrackableObject<Extension>,
                  public extensions::ExtensionRegistryObserver {
 public:
  static mate::Handle<Extension> Create(v8::Isolate* isolate,
                                    content::BrowserContext* browser_context);
  // mate::TrackableObject:
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  static bool HandleURLOverride(GURL* url,
                                     content::BrowserContext* browser_context);
  static bool HandleURLOverrideReverse(GURL* url,
                                     content::BrowserContext* browser_context);

  static bool IsBackgroundPageUrl(GURL url,
                                    content::BrowserContext* browser_context);
  static bool IsBackgroundPageWebContents(content::WebContents* web_contents);
  static bool IsBackgroundPage(const WebContents* web_contents);
  static v8::Local<v8::Value> TabValue(v8::Isolate* isolate,
                                         WebContents* web_contents);

 protected:
  Extension(v8::Isolate* isolate, AtomBrowserContext* browser_context);
  ~Extension() override;

  void Load(mate::Arguments* args);
  void AddExtension(scoped_refptr<extensions::Extension> extension);
  void OnExtensionReady(content::BrowserContext* browser_context,
                        const extensions::Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                    const extensions::Extension* extension,
                    extensions::UnloadedExtensionInfo::Reason reason) override;


  void Disable(const std::string& extension_id);
  void Enable(const std::string& extension_id);

 private:
  scoped_refptr<AtomBrowserContext> browser_context_;

  DISALLOW_COPY_AND_ASSIGN(Extension);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_EXTENSION_H_
