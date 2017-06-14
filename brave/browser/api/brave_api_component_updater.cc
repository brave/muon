// Copyright 2016 Brave authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/api/brave_api_component_updater.h"

#include "base/base64.h"
#include "brave/browser/component_updater/default_extensions.h"
#include "brave/browser/component_updater/extension_installer_traits.h"
#include "brave/browser/component_updater/widevine_cdm_component_installer.h"
#include "chrome/browser/browser_process_impl.h"
#include "components/component_updater/component_updater_service.h"
#include "components/update_client/crx_update_item.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

void ComponentsUI::OnDemandUpdate(
    component_updater::ComponentUpdateService* cus,
    const std::string& component_id) {
  cus->GetOnDemandUpdater().OnDemandUpdate(component_id,
      component_updater::Callback());
}

bool
ComponentsUI::GetComponentDetails(const std::string& component_id,
    update_client::CrxUpdateItem* item) const {
  return GetCUSForID(component_id)
    ->GetComponentDetails(component_id, item);
}

// Due to licensing reasons, some components are handled through
// Google's servers. i.e. Widevine.
component_updater::ComponentUpdateService* ComponentsUI::GetCUSForID(
    const std::string& component_id) const {
  if (component_id == kWidevineId) {
    return g_browser_process->component_updater();
  }
  return static_cast<BrowserProcessImpl*>(g_browser_process)->
      brave_component_updater();
}


namespace brave {

namespace api {

ComponentUpdater::ComponentUpdater(v8::Isolate* isolate) : weak_factory_(this) {
  Init(isolate);
}

ComponentUpdater::~ComponentUpdater() {
}

void ComponentUpdater::OnEvent(Events event, const std::string& component_id) {
  update_client::CrxUpdateItem item;
  GetComponentDetails(component_id, &item);
  switch (event) {
    case Events::COMPONENT_CHECKING_FOR_UPDATES:
      Emit("component-checking-for-updates", component_id);
      break;
    case Events::COMPONENT_WAIT:
      Emit("component-wait", component_id);
      break;
    case Events::COMPONENT_UPDATE_FOUND:
      Emit("component-update-found", component_id);
      break;
    case Events::COMPONENT_UPDATE_READY:
      Emit("component-update-ready", component_id);
      break;
    case Events::COMPONENT_UPDATED:
      Emit("component-update-updated", component_id,
          item.component.version.GetString());
      break;
    case Events::COMPONENT_NOT_UPDATED:
      Emit("component-not-updated", component_id);
      break;
    case Events::COMPONENT_UPDATE_DOWNLOADING:
      Emit("component-update-downloading", component_id);
      break;
  }
}

void ComponentUpdater::RegisterComponentForUpdate(
    const std::string& public_key,
    const base::Closure& registered_callback,
    const ReadyCallback& ready_callback) {
  brave::RegisterExtension(
      static_cast<BrowserProcessImpl*>(g_browser_process)->
          brave_component_updater(),
      public_key, registered_callback, ready_callback);
}

void ComponentUpdater::OnComponentRegistered(const std::string& component_id) {
  Emit("component-registered", component_id);
}

void ComponentUpdater::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir) {
  Emit("component-ready", component_id,
    std::string(install_dir.value().begin(), install_dir.value().end()));
}

void ComponentUpdater::RegisterComponent(mate::Arguments* args) {
  static bool registeredObserver = false;
  if (!registeredObserver) {
    static_cast<BrowserProcessImpl*>(g_browser_process)->
        brave_component_updater()->AddObserver(this);
    g_browser_process->component_updater()->AddObserver(this);
    registeredObserver = true;
  }

  std::string component_id;
  if (!args->GetNext(&component_id)) {
    args->ThrowError("`componentId` is a required field");
    return;
  }

  base::Closure registered_callback =
    base::Bind(&ComponentUpdater::OnComponentRegistered,
               weak_factory_.GetWeakPtr(), component_id);
  ReadyCallback ready_callback =
    base::Bind(&ComponentUpdater::OnComponentReady,
               weak_factory_.GetWeakPtr(), component_id);

  std::string public_key_string;

  if (component_id == kOnePasswordId) {
    base::Base64Encode(kOnePasswordPublicKeyStr, &public_key_string);
  } else if (component_id == kDashlaneId) {
    base::Base64Encode(kDashlanePublicKeyStr, &public_key_string);
  } else if (component_id == kLastPassId) {
    base::Base64Encode(kLastPassPublicKeyStr, &public_key_string);
  } else if (component_id == kPDFJSId) {
    base::Base64Encode(kPDFJSPublicKeyStr, &public_key_string);
  } else if (component_id == kPocketId) {
    base::Base64Encode(kPocketPublicKeyStr, &public_key_string);
  } else if (component_id == kVimiumId) {
    base::Base64Encode(kVimiumPublicKeyStr, &public_key_string);
  } else if (component_id == kEnpassId) {
    base::Base64Encode(kEnpassPublicKeyStr, &public_key_string);
  } else if (component_id == kBitwardenId) {
    base::Base64Encode(kBitwardenPublicKeyStr, &public_key_string);
  } else if (component_id == kHoneyId) {
    base::Base64Encode(kHoneyPublicKeyStr, &public_key_string);
  } else if (component_id == kWidevineId) {
    brave::RegisterWidevineCdmComponent(
        g_browser_process->component_updater(),
        registered_callback, ready_callback);
    return;
  } else {
    if (!args->GetNext(&public_key_string)) {
      args->ThrowError("`publicKeyString` is a required field");
      return;
    }
    std::string base64Test;
    if (public_key_string.empty() ||
        !base::Base64Decode(public_key_string, &base64Test)) {
      args->ThrowError("`publicKeyString` is invalid");
      return;
    }
  }

  RegisterComponentForUpdate(
      public_key_string, registered_callback, ready_callback);
}

std::vector<std::string> ComponentUpdater::GetComponentIDs() {
  std::vector<std::string> components =
      g_browser_process->component_updater()->GetComponentIDs();
  std::vector<std::string> brave_components =
      static_cast<BrowserProcessImpl*>(g_browser_process)->
          brave_component_updater()->GetComponentIDs();
  components.insert(components.end(),
      brave_components.begin(), brave_components.end());
  return components;
}

void ComponentUpdater::CheckNow(const std::string& component_id) {
  OnDemandUpdate(GetCUSForID(component_id), component_id);
}

// static
mate::Handle<ComponentUpdater> ComponentUpdater::Create(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new ComponentUpdater(isolate));
}

// static
void ComponentUpdater::BuildPrototype(v8::Isolate* isolate,
                              v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "ComponentUpdater"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
    .SetMethod("registerComponent", &ComponentUpdater::RegisterComponent)
    .SetMethod("registeredComponentIDs", &ComponentUpdater::GetComponentIDs)
    .SetMethod("checkNow", &ComponentUpdater::CheckNow);
}

}  // namespace api

}  // namespace brave

namespace {

using brave::api::ComponentUpdater;

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("ComponentUpdater",
      ComponentUpdater::GetConstructor(isolate)->GetFunction());
  dict.Set("componentUpdater", ComponentUpdater::Create(isolate));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_component_updater, Initialize)
