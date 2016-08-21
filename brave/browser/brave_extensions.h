// Copyright (c) 2016 Brave.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_BRAVE_EXTENSIONS_H_
#define BRAVE_BROWSER_BRAVE_EXTENSIONS_H_

#include <string>
#include "atom/browser/atom_browser_context.h"
#include "base/memory/singleton.h"
#include "extensions/common/manifest.h"
#include "extensions/common/extension_set.h"
#include "native_mate/handle.h"

namespace mate {
class Dictionary;
}

namespace base {
class FilePath;
}

namespace brave {

/*
 * Handles a collection of extensions
 */

class BraveExtensions {
 public:
  BraveExtensions();
  virtual ~BraveExtensions();

  static BraveExtensions* Get();
  const extensions::Extension* GetByID(const std::string& extension_id);
  void Insert(extensions::Extension* extension);

  const extensions::ExtensionSet& extensions() const { return extensions_; }

  void Enable(const std::string& extension_id);
  void Disable(const std::string& extension_id);
  mate::Dictionary Load(v8::Isolate* isolate,
    const base::FilePath& path,
    const base::DictionaryValue& manifest,
    const extensions::Manifest::Location& manifest_location,
    int flags);
  const base::ListValue *
    GetExtensionsWebContents(content::WebContents* web_contents);
  void Install(
    const scoped_refptr<const extensions::Extension>& extension);
  void InstallCrx(const base::FilePath& path);
  void UpdateFromCrx(const base::FilePath& path,
    const std::string& extension_id);

  static bool IsBackgroundPageUrl(GURL url,
                                 content::BrowserContext* browser_context);
  static bool IsBackgroundPageWebContents(content::WebContents* web_contents);

 private:
   extensions::ExtensionSet extensions_;
};

}  // namespace

#endif  // ATOM_BROWSER_BROWSER_H_
