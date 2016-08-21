// Copyright (c) 2016 Brave.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/browser/brave_extensions.h"

#include <vector>
#include "atom/browser/extensions/atom_notification_types.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "brave/browser/crx_installer.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/one_shot_event.h"
#include "native_mate/dictionary.h"


namespace {

// static
scoped_refptr<extensions::Extension> LoadExtension(
    const base::FilePath& path,
    const base::DictionaryValue& manifest,
    const extensions::Manifest::Location& manifest_location,
    int flags,
    std::string* error) {
  scoped_refptr<extensions::Extension> extension(extensions::Extension::Create(
      path, manifest_location, manifest, flags, error));
  if (!extension.get())
    return nullptr;

  std::vector<extensions::InstallWarning> warnings;
  if (!extensions::file_util::ValidateExtension(extension.get(),
                                                error,
                                                &warnings))
    return nullptr;
  extension->AddInstallWarnings(warnings);

  return extension;
}

}  // namespace

namespace brave {

BraveExtensions::BraveExtensions() {
}

BraveExtensions::~BraveExtensions() {
}

// static
BraveExtensions* BraveExtensions::Get() {
  return atom::AtomBrowserMainParts::Get()->brave_extensions();
}

const extensions::Extension*
BraveExtensions::GetByID(const std::string& extension_id) {
  return extensions_.GetByID(extension_id);
}

void BraveExtensions::Insert(extensions::Extension* extension) {
  extensions_.Insert(extension);
}

void BraveExtensions::Enable(const std::string& extension_id) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI))
    content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
        base::Bind(&BraveExtensions::Enable, base::Unretained(this), extension_id));

  const extensions::Extension* extension = GetByID(extension_id);

  if (!extension)
    return;

  content::NotificationService::current()->Notify(
        atom::NOTIFICATION_ENABLE_USER_EXTENSION_REQUEST,
        content::Source<BraveExtensions>(this),
        content::Details<const extensions::Extension>(extension));
}

void BraveExtensions::Disable(const std::string& extension_id) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI))
    content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
        base::Bind(&BraveExtensions::Disable, base::Unretained(this), extension_id));

  const extensions::Extension* extension = GetByID(extension_id);

  if (!extension)
    return;

  content::NotificationService::current()->Notify(
        atom::NOTIFICATION_DISABLE_USER_EXTENSION_REQUEST,
        content::Source<BraveExtensions>(this),
        content::Details<const extensions::Extension>(extension));
}

mate::Dictionary BraveExtensions::Load(
  v8::Isolate* isolate,
  const base::FilePath& path,
  const base::DictionaryValue& manifest,
  const extensions::Manifest::Location& manifest_location,
  int flags) {
  std::string error;
  scoped_refptr<extensions::Extension> extension;

  if (manifest.empty()) {
    extension = extensions::file_util::LoadExtension(path,
                                                    manifest_location,
                                                    flags,
                                                    &error);
  } else {
    extension = LoadExtension(path,
                              manifest,
                              manifest_location,
                              flags,
                              &error);
  }

  mate::Dictionary install_info = mate::Dictionary::CreateEmpty(isolate);
  if (error.empty()) {
    Install(extension);
    install_info.Set("name", extension->non_localized_name());
    install_info.Set("id", extension->id());
    install_info.Set("url", extension->url().spec());
    install_info.Set("path", extension->path());
    install_info.Set("version", extension->VersionString());
    install_info.Set("description", extension->description());
  } else {
    install_info.Set("error", error);
  }
  return install_info;
}

const base::ListValue*
BraveExtensions::GetExtensionsWebContents(content::WebContents* web_contents) {
  auto browser_context = web_contents->GetBrowserContext();
  const extensions::ExtensionSet& extensions =
      extensions::ExtensionRegistry::Get(browser_context)->enabled_extensions();
      //extensions::ExtensionRegistry::Get(browser_context)->ready_extensions();
  auto* result = new base::ListValue();
  for (extensions::ExtensionSet::const_iterator iter =
           extensions.begin();
       iter != extensions.end(); ++iter) {
    const extensions::Extension* extension = iter->get();
    auto* dict = new base::DictionaryValue();
    dict->SetString("name", extension->non_localized_name());
    dict->SetString("id", extension->id());
    dict->SetString("url", extension->url().spec());
    dict->SetString("path", extension->path().value());
    dict->SetString("version", extension->VersionString());
    dict->SetString("description", extension->description());
    result->Append(dict);
  }
  return result;
}

void BraveExtensions::Install(
    const scoped_refptr<const extensions::Extension>& extension) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI))
    content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
        base::Bind(&BraveExtensions::Install,
          base::Unretained(this), extension));

  content::NotificationService::current()->Notify(
        extensions::NOTIFICATION_CRX_INSTALLER_DONE,
        content::Source<BraveExtensions>(this),
        content::Details<const extensions::Extension>(extension.get()));
}

CrxInstaller installer;
void BraveExtensions::InstallCrx(const base::FilePath& path) {
  installer.InstallCrx(path);
}

void BraveExtensions::UpdateFromCrx(const base::FilePath& path,
  const std::string& extension_id) {
  LOG(INFO) << "=======Passing extension Id: " << extension_id.c_str();
  installer.UpdateFromCrx(path, extension_id);
}

// static
bool BraveExtensions::IsBackgroundPageUrl(GURL url,
                    content::BrowserContext* browser_context) {
  if (url.scheme() != "chrome-extension")
    return false;

  if (extensions::ExtensionSystem::Get(browser_context)
      ->ready().is_signaled()) {
    const extensions::Extension* extension =
        extensions::ExtensionRegistry::Get(browser_context)->
            enabled_extensions().GetExtensionOrAppByURL(url);
    if (extension &&
        url == extensions::BackgroundInfo::GetBackgroundURL(extension))
      return true;
  }

  return false;
}

// static
bool BraveExtensions::IsBackgroundPageWebContents(
    content::WebContents* web_contents) {
  auto browser_context = web_contents->GetBrowserContext();
  auto url = web_contents->GetURL();

  return IsBackgroundPageUrl(url, browser_context);
}

}  // namespace brave
