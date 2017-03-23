// Copyright 2016 Brave authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_API_BRAVE_API_EXTENSION_H_
#define BRAVE_BROWSER_API_BRAVE_API_EXTENSION_H_

#include <memory>
#include <string>

#include "brave/browser/brave_browser_context.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/manifest.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"

namespace base {
class FilePath;
}

namespace content {
class BrowserContext;
}

namespace extensions {
class Extension;
}

namespace brave {

namespace api {

class Extension : public gin::Wrappable<Extension>,
                         extensions::ExtensionRegistryObserver {
 public:
  static gin::WrapperInfo kWrapperInfo;
  static gin::Handle<Extension> Create(v8::Isolate* isolate,
                                      content::BrowserContext* browser_context);

  gin::ObjectTemplateBuilder
      GetObjectTemplateBuilder(v8::Isolate* isolate) override;
  static bool HandleURLOverride(GURL* url,
                                     content::BrowserContext* browser_context);
  static bool HandleURLOverrideReverse(GURL* url,
                                     content::BrowserContext* browser_context);

  static bool IsBackgroundPageUrl(GURL url,
                                    content::BrowserContext* browser_context);
  static bool IsBackgroundPageWebContents(content::WebContents* web_contents);

 protected:
  Extension(v8::Isolate* isolate, BraveBrowserContext* browser_context);
  ~Extension() override;

  void NotifyLoadOnUIThread(scoped_refptr<extensions::Extension> extension);
  void NotifyErrorOnUIThread(const std::string& error);
  void LoadOnFILEThread(const base::FilePath path,
      std::unique_ptr<base::DictionaryValue> manifest,
      extensions::Manifest::Location manifest_location,
      int flags);
  void Load(gin::Arguments* args);
  void AddExtension(scoped_refptr<extensions::Extension> extension);
  void OnExtensionReady(content::BrowserContext* browser_context,
                        const extensions::Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                    const extensions::Extension* extension,
                    extensions::UnloadedExtensionInfo::Reason reason) override;


  void Disable(const std::string& extension_id);
  void Enable(const std::string& extension_id);
  v8::Isolate* isolate() { return isolate_; }

 private:
  v8::Isolate* isolate_;  // not owned
  BraveBrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(Extension);
};

}  // namespace api

}  // namespace brave

#endif  // BRAVE_BROWSER_API_BRAVE_API_EXTENSION_H_
