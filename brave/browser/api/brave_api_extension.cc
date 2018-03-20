// Copyright 2016 Brave authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/api/brave_api_extension.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "atom/browser/extensions/atom_component_extensions.h"
#include "atom/browser/extensions/atom_extension_system.h"
#include "atom/browser/extensions/tab_helper.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/node_includes.h"
#include "base/files/file_path.h"
#include "base/json/json_string_value_serializer.h"
#include "base/strings/string_util.h"
#include "brave/common/converters/callback_converter.h"
#include "brave/common/converters/file_path_converter.h"
#include "brave/common/converters/gurl_converter.h"
#include "brave/common/converters/value_converter.h"
#include "chrome/common/chrome_paths.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "content/public/renderer/v8_value_converter.h"
#include "extensions/browser/component_extension_resource_manager.h"
#include "extensions/browser/disable_reason.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/notification_types.h"
#include "extensions/browser/process_manager.h"
#include "extensions/common/extension.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/one_shot_event.h"
#include "extensions/strings/grit/extensions_strings.h"
#include "gin/dictionary.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"


using base::Callback;
using content::BrowserURLHandler;
using content::V8ValueConverter;

namespace gin {

template<>
struct Converter<extensions::Manifest::Location> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     extensions::Manifest::Location* out) {
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;

    if (type == "internal")
      *out = extensions::Manifest::Location::INTERNAL;
    else if (type == "external_pref")
      *out = extensions::Manifest::Location::EXTERNAL_PREF;
    else if (type == "external_registry")
      *out = extensions::Manifest::Location::EXTERNAL_REGISTRY;
    else if (type == "unpacked")
      *out = extensions::Manifest::Location::UNPACKED;
    else if (type == "component")
      *out = extensions::Manifest::Location::COMPONENT;
    else if (type == "external_pref_download")
      *out = extensions::Manifest::Location::EXTERNAL_PREF_DOWNLOAD;
    else if (type == "external_policy_download")
      *out = extensions::Manifest::Location::EXTERNAL_POLICY_DOWNLOAD;
    else if (type == "command_line")
      *out = extensions::Manifest::Location::COMMAND_LINE;
    else if (type == "external_policy")
      *out = extensions::Manifest::Location::EXTERNAL_POLICY;
    else if (type == "external_component")
      *out = extensions::Manifest::Location::EXTERNAL_COMPONENT;
    else
      *out = extensions::Manifest::Location::INVALID_LOCATION;
    return true;
  }
};

}  // namespace gin

namespace {

scoped_refptr<extensions::Extension> LoadExtension(const base::FilePath& path,
    const base::DictionaryValue& manifest,
    const extensions::Manifest::Location& manifest_location,
    int flags,
    std::string* error) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::FILE);

  scoped_refptr<extensions::Extension> extension(extensions::Extension::Create(
      path, manifest_location, manifest, flags, error));
  if (!extension.get())
    return NULL;

  std::vector<extensions::InstallWarning> warnings;

  int resource_id;
  if (!IsComponentExtension(path, &resource_id)) {
    // Component extensions contained inside the resources pak fail manifest
    // validation so we skip validation.
    if (!extensions::file_util::ValidateExtension(extension.get(),
                                                  error,
                                                  &warnings)) {
      return NULL;
    }
  }
  extension->AddInstallWarnings(warnings);

  return extension;
}

std::map<std::string,
  base::Callback<GURL(const GURL&)>> url_override_callbacks_;
std::map<std::string,
  base::Callback<GURL(const GURL&)>> reverse_url_override_callbacks_;
}  // namespace

namespace brave {

namespace api {

// Extension ===================================================================

gin::WrapperInfo Extension::kWrapperInfo = { gin::kEmbedderNativeGin };

// static
gin::Handle<Extension> Extension::Create(v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  auto original_context = extensions::ExtensionsBrowserClient::Get()->
        GetOriginalContext(browser_context);
  return gin::CreateHandle(isolate,
      new Extension(isolate,
          static_cast<BraveBrowserContext*>(original_context)));
}

gin::ObjectTemplateBuilder Extension::GetObjectTemplateBuilder(
                                                        v8::Isolate* isolate) {
  return gin::Wrappable<Extension>::GetObjectTemplateBuilder(isolate)
      .SetMethod("load", &Extension::Load)
      .SetMethod("enable", &Extension::Enable)
      .SetMethod("disable", &Extension::Disable)
      .SetMethod("setURLHandler", &Extension::SetURLHandler)
      .SetMethod("setReverseURLHandler", &Extension::SetReverseURLHandler);
}

Extension::Extension(v8::Isolate* isolate,
                 BraveBrowserContext* browser_context)
    : isolate_(isolate),
      browser_context_(browser_context) {
  extensions::ExtensionRegistry::Get(browser_context_)->AddObserver(this);
}

Extension::~Extension() {
  if (extensions::ExtensionRegistry::Get(browser_context_)) {
    extensions::ExtensionRegistry::Get(browser_context_)->
        RemoveObserver(this);
  }
}

std::unique_ptr<base::DictionaryValue> Extension::LoadManifest(
    const base::FilePath& extension_root,
    std::string* error) {
  int resource_id;
  if (IsComponentExtension(extension_root, &resource_id)) {
    const ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    base::StringPiece manifest_contents = rb.GetRawDataResource(resource_id);

    JSONStringValueDeserializer deserializer(manifest_contents);
    std::unique_ptr<base::Value> root(deserializer.Deserialize(NULL, error));
    if (!root.get()) {
      if (error->empty()) {
        // If |error| is empty, than the file could not be read.
        // It would be cleaner to have the JSON reader give a specific error
        // in this case, but other code tests for a file error with
        // error->empty().  For now, be consistent.
        *error = l10n_util::GetStringUTF8(IDS_EXTENSION_MANIFEST_UNREADABLE);
      } else {
        *error = base::StringPrintf(
            "%s  %s", extensions::manifest_errors::kManifestParseError,
            error->c_str());
      }
      return NULL;
    }

    if (!root->is_dict()) {
      *error = l10n_util::GetStringUTF8(IDS_EXTENSION_MANIFEST_INVALID);
      return NULL;
    }

    return base::DictionaryValue::From(std::move(root));
  } else {
    return extensions::file_util::LoadManifest(extension_root, error);
  }
}

void Extension::LoadOnFILEThread(const base::FilePath path,
    std::unique_ptr<base::DictionaryValue> manifest,
    extensions::Manifest::Location manifest_location,
    int flags) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::FILE);

  std::string error;
  if (manifest->empty()) {
    manifest = LoadManifest(path, &error);
  }

  if (!manifest || !error.empty()) {
    content::BrowserThread::PostTask(
          content::BrowserThread::UI, FROM_HERE,
          base::Bind(&Extension::NotifyErrorOnUIThread,
              base::Unretained(this), error));
  } else {
    scoped_refptr<extensions::Extension> extension = LoadExtension(path,
                              *manifest,
                              manifest_location,
                              flags,
                              &error);

    if (!extension || !error.empty()) {
      content::BrowserThread::PostTask(
          content::BrowserThread::UI, FROM_HERE,
          base::Bind(&Extension::NotifyErrorOnUIThread,
              base::Unretained(this), error));
    } else {
      content::BrowserThread::PostTask(
          content::BrowserThread::UI, FROM_HERE,
          base::Bind(&Extension::NotifyLoadOnUIThread,
              base::Unretained(this), base::Passed(&extension)));
    }
  }
}

void Extension::NotifyLoadOnUIThread(
    scoped_refptr<extensions::Extension> extension) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  extensions::ExtensionSystem::Get(browser_context_)->ready().Post(
        FROM_HERE,
        base::Bind(&Extension::AddExtension,
          // GetWeakPtr()
          base::Unretained(this), base::Passed(&extension)));
}

void Extension::NotifyErrorOnUIThread(const std::string& error) {
  node::Environment* env = node::Environment::GetCurrent(isolate());
  if (!env)
    return;

  mate::EmitEvent(isolate(),
                env->process_object(),
                "extension-load-error",
                error);
}

void Extension::Load(gin::Arguments* args) {
  base::FilePath path;
  args->GetNext(&path);

  base::DictionaryValue manifest;
  args->GetNext(&manifest);

  extensions::Manifest::Location manifest_location =
      extensions::Manifest::Location::UNPACKED;
  args->GetNext(&manifest_location);

  int flags = 0;
  args->GetNext(&flags);

  std::unique_ptr<base::DictionaryValue> manifest_copy =
      manifest.CreateDeepCopy();

  content::BrowserThread::PostTask(
        content::BrowserThread::FILE, FROM_HERE,
        base::Bind(&Extension::LoadOnFILEThread,
            base::Unretained(this),
            path, Passed(&manifest_copy), manifest_location, flags));
}

void Extension::AddExtension(scoped_refptr<extensions::Extension> extension) {
  auto extension_service =
      extensions::ExtensionSystem::Get(browser_context_)->
          extension_service();
  if (!extensions::ExtensionRegistry::Get(
        browser_context_)->GetInstalledExtension(extension->id())) {
    extension_service->AddExtension(extension.get());
  }
}

void Extension::OnExtensionReady(content::BrowserContext* browser_context,
                                const extensions::Extension* extension) {
  gin::Dictionary install_info = gin::Dictionary::CreateEmpty(isolate());
  install_info.Set("name", extension->non_localized_name());
  install_info.Set("id", extension->id());
  install_info.Set("url", extension->url().spec());
  install_info.Set("base_path", extension->path().value());
  install_info.Set("version", extension->VersionString());
  install_info.Set("description", extension->description());
  auto manifest_copy = extension->manifest()->value()->CreateDeepCopy();
  std::unique_ptr<V8ValueConverter> converter = V8ValueConverter::Create();

  install_info.Set("manifest", converter->ToV8Value(manifest_copy.get(),
      isolate()->GetCurrentContext()));

  node::Environment* env = node::Environment::GetCurrent(isolate());
  if (!env)
    return;

  mate::EmitEvent(isolate(),
                  env->process_object(),
                  "EXTENSION_READY_INTERNAL",
                  install_info);
}

void Extension::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionReason reason) {
  node::Environment* env = node::Environment::GetCurrent(isolate());
  if (!env)
    return;

  mate::EmitEvent(isolate(),
                  env->process_object(),
                  "extension-unloaded",
                  extension->id());
}

void Extension::Disable(const std::string& extension_id) {
  auto extension_service =
      extensions::ExtensionSystem::Get(browser_context_)->
          extension_service();
  if (extension_service && extensions::ExtensionRegistry::Get(
        browser_context_)->GetInstalledExtension(extension_id)) {
    extension_service->DisableExtension(
        extension_id, extensions::disable_reason::DISABLE_USER_ACTION);
  }
}

void Extension::Enable(const std::string& extension_id) {
  auto extension_service =
      extensions::ExtensionSystem::Get(browser_context_)->
          extension_service();
  if (extension_service && extensions::ExtensionRegistry::Get(
        browser_context_)->GetInstalledExtension(extension_id)) {
    extension_service->EnableExtension(
        extension_id);
  }
}

// static
bool Extension::IsBackgroundPageUrl(GURL url,
                    content::BrowserContext* browser_context) {
  if (!url.is_valid() || url.scheme() != "chrome-extension")
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
bool Extension::IsBackgroundPageWebContents(
    content::WebContents* web_contents) {
  auto browser_context = web_contents->GetBrowserContext();
  auto url = web_contents->GetURL();
  return IsBackgroundPageUrl(url, browser_context) &&
      web_contents == MaybeCreateBackgroundContents(browser_context, url);
}

// static
content::WebContents* Extension::MaybeCreateBackgroundContents(
    content::BrowserContext* browser_context,
    const GURL& target_url) {
  if (!extensions::ExtensionSystem::Get(browser_context)->ready().is_signaled())
    return nullptr;

  if (!target_url.is_valid() ||
      !IsBackgroundPageUrl(target_url, browser_context))
    return nullptr;

  auto extensions_service =
      extensions::ExtensionSystem::Get(browser_context)->extension_service();

  if (!extensions_service || !extensions_service->is_ready())
    return nullptr;

  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(browser_context)->enabled_extensions()
                                            .GetExtensionOrAppByURL(target_url);
  if (!extension)
    return nullptr;

  // Only allow a single background contents per app.
  extensions::ExtensionHost* extension_host =
      extensions::ProcessManager::Get(browser_context)
          ->GetBackgroundHostForExtension(extension->id());
  if (!extension_host)
    return nullptr;

  return extension_host->host_contents();
}

void Extension::SetURLHandler(gin::Arguments* args) {
  std::string extension_id;
  if (!args->GetNext(&extension_id)) {
    args->ThrowTypeError("`extension_id` must be a string");
    return;
  }

  base::Callback<GURL(const GURL&)> callback;
  if (!args->GetNext(&callback)) {
    args->ThrowTypeError("`callback` must be a function");
    return;
  }

  url_override_callbacks_[extension_id] = callback;
}

void Extension::SetReverseURLHandler(gin::Arguments* args) {
  std::string extension_id;
  if (!args->GetNext(&extension_id)) {
    args->ThrowTypeError("`extension_id` must be a string");
    return;
  }

  base::Callback<GURL(const GURL&)> callback;
  if (!args->GetNext(&callback)) {
    args->ThrowTypeError("`callback` must be a function");
    return;
  }

  reverse_url_override_callbacks_[extension_id] = callback;
}

// static
bool Extension::HandleURLOverride(GURL* url,
        content::BrowserContext* browser_context) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(browser_context)->enabled_extensions()
          .GetExtensionOrAppByURL(*url);
  if (!extension)
    return false;

  if (!base::ContainsKey(url_override_callbacks_, extension->id()))
    return false;

  GURL new_url = url_override_callbacks_[extension->id()].Run(*url);
  if (new_url != GURL()) {
    *url = new_url;
    return true;
  }

  return false;
}

bool Extension::HandleURLOverrideReverse(GURL* url,
          content::BrowserContext* browser_context) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(browser_context)->enabled_extensions()
          .GetExtensionOrAppByURL(*url);
  if (!extension)
    return false;

  if (!base::ContainsKey(reverse_url_override_callbacks_, extension->id()))
    return false;

  GURL new_url = reverse_url_override_callbacks_[extension->id()].Run(*url);
  if (new_url != GURL()) {
    *url = new_url;
    return true;
  }

  return false;
}

}  // namespace api

}  // namespace brave
