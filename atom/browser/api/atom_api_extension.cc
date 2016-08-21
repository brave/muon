// Copyright (c) 2016 Brave.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_extension.h"

#include <string>
#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/extensions/tab_helper.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/native_mate_converters/v8_value_converter.h"
#include "brave/browser/brave_extensions.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/notification_types.h"
#include "native_mate/dictionary.h"

namespace mate {

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

template<>
struct Converter<const base::ListValue*> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const base::ListValue* val) {
    std::unique_ptr<atom::V8ValueConverter>
        converter(new atom::V8ValueConverter);
    return converter->ToV8Value(val, isolate->GetCurrentContext());
  }
};

}  // namespace mate


namespace atom {

namespace api {

Extension::Extension(v8::Isolate* isolate) {
  registrar_.Add(this,
                 extensions::NOTIFICATION_EXTENSION_LOADED_DEPRECATED,
                 content::NotificationService::AllBrowserContextsAndSources());
  registrar_.Add(this,
                 extensions::NOTIFICATION_CRX_INSTALLER_DONE,
                 content::NotificationService::AllBrowserContextsAndSources());
  registrar_.Add(this,
                 extensions::NOTIFICATION_EXTENSION_ENABLED,
                 content::NotificationService::AllBrowserContextsAndSources());
  registrar_.Add(this,
                 content::NOTIFICATION_WEB_CONTENTS_RENDER_VIEW_HOST_CREATED,
                 content::NotificationService::AllBrowserContextsAndSources());
  Init(isolate);
}

Extension::~Extension() {}

void Extension::Observe(
    int type, const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case extensions::NOTIFICATION_EXTENSION_LOADED_DEPRECATED: {
      extensions::Extension* extension =
          content::Details<extensions::Extension>(details).ptr();
      brave::BraveExtensions::Get()->Insert(extension);
      break;
    }
    case content::NOTIFICATION_WEB_CONTENTS_RENDER_VIEW_HOST_CREATED: {
      content::WebContents* web_contents =
          content::Source<content::WebContents>(source).ptr();
      auto browser_context = web_contents->GetBrowserContext();
      auto url = web_contents->GetURL();

      // Make sure background pages get a webcontents api wrapper so they can
      // communicate via IPC.
      if (brave::BraveExtensions::IsBackgroundPageUrl(url, browser_context)) {
        WebContents::CreateFrom(isolate(), web_contents);
      }
      break;
    }
    case extensions::NOTIFICATION_CRX_INSTALLER_DONE: {
      Emit("crx-installer-done");
      break;
    }
    case extensions::NOTIFICATION_EXTENSION_ENABLED: {
      Emit("extension-enabled");
      break;
    }
  }
}

// static
bool Extension::HandleURLOverride(GURL* url,
        content::BrowserContext* browser_context) {
  return false;
}

bool Extension::HandleURLOverrideReverse(GURL* url,
          content::BrowserContext* browser_context) {
  return false;
}

// static
mate::Handle<Extension> Extension::Create(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new Extension(isolate));
}
// static
bool Extension::IsBackgroundPage(const WebContents* web_contents) {
  return brave::BraveExtensions::Get()->
    IsBackgroundPageWebContents(web_contents->web_contents());
}

// static
v8::Local<v8::Value> Extension::TabValue(v8::Isolate* isolate,
                    WebContents* web_contents) {
  std::unique_ptr<base::DictionaryValue> value(
      extensions::TabHelper::CreateTabValue(web_contents->web_contents()));
  return mate::ConvertToV8(isolate, *value);
}

// static
const base::ListValue*
Extension::GetExtensions(const WebContents* web_contents) {
  return brave::BraveExtensions::Get()->
    GetExtensionsWebContents(web_contents->web_contents());
}

// static
void Extension::BuildPrototype(
    v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "Extension"));
  auto brave_extensions = base::Unretained(brave::BraveExtensions::Get());
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("enable",
          base::Bind(&brave::BraveExtensions::Enable, brave_extensions))
      .SetMethod("disable",
          base::Bind(&brave::BraveExtensions::Disable, brave_extensions))
      .SetMethod("load",
          base::Bind(&brave::BraveExtensions::Load, brave_extensions))
      .SetMethod("tabValue", &atom::api::Extension::TabValue)
      .SetMethod("getExtensions", &atom::api::Extension::GetExtensions)
      .SetMethod("installCrx",
          base::Bind(&brave::BraveExtensions::InstallCrx, brave_extensions))
      .SetMethod("updateFromCrx",
          base::Bind(&brave::BraveExtensions::UpdateFromCrx, brave_extensions))
      .SetMethod("isBackgroundPage", &atom::api::Extension::IsBackgroundPage);
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("Extension",
      atom::api::Extension::GetConstructor(isolate)->GetFunction());
  dict.Set("extension", atom::api::Extension::Create(isolate));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_extension, Initialize)
