// Copyright 2016 Brave authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/api/brave_api_component_updater.h"

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

void ComponentUpdater::RegisterComponent(const std::string& component_id) {
  static bool registeredObserver = false;
  if (!registeredObserver) {
    static_cast<BrowserProcessImpl*>(g_browser_process)->
        brave_component_updater()->AddObserver(this);
    g_browser_process->component_updater()->AddObserver(this);
    registeredObserver = true;
  }
  base::Closure registered_callback =
    base::Bind(&ComponentUpdater::OnComponentRegistered,
               weak_factory_.GetWeakPtr(), component_id);
  ReadyCallback ready_callback =
    base::Bind(&ComponentUpdater::OnComponentReady,
               weak_factory_.GetWeakPtr(), component_id);
  if (component_id == kOnePasswordId) {
    RegisterComponentForUpdate(
        kOnePasswordPublicKeyStr, registered_callback, ready_callback);
  } else if (component_id == kDashlaneId) {
    RegisterComponentForUpdate(
        kDashlanePublicKeyStr, registered_callback, ready_callback);
  } else if (component_id == kLastPassId) {
    RegisterComponentForUpdate(
        kLastPassPublicKeyStr, registered_callback, ready_callback);
  } else if (component_id == kPDFJSId) {
    RegisterComponentForUpdate(
        kPDFJSPublicKeyStr, registered_callback, ready_callback);
  } else if (component_id == kPocketId) {
    RegisterComponentForUpdate(
        kPocketPublicKeyStr, registered_callback, ready_callback);
  } else if (component_id == kVimiumId) {
    RegisterComponentForUpdate(
        kVimiumPublicKeyStr, registered_callback, ready_callback);
  } else if (component_id == kEnpassId) {
    RegisterComponentForUpdate(
        kEnpassPublicKeyStr, registered_callback, ready_callback);
  } else if (component_id == kBitwardenId) {
    RegisterComponentForUpdate(
        kBitwardenPublicKeyStr, registered_callback, ready_callback);
  } else if (component_id == kHoneyId) {
    RegisterComponentForUpdate(
        kHoneyPublicKeyStr, registered_callback, ready_callback);
  } else if (component_id == kPinterestId) {
    RegisterComponentForUpdate(
        kPinterestPublicKeyStr, registered_callback, ready_callback);
  } else if (component_id == kWidevineId) {
    brave::RegisterWidevineCdmComponent(
        g_browser_process->component_updater(),
        registered_callback, ready_callback);
  }
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
