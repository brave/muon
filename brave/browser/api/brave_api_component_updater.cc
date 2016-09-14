// Copyright 2016 Brave authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/api/brave_api_component_updater.h"

#include "atom/browser/api/atom_api_extension.h"
#include "brave/browser/component_updater/default_extensions.h"
#include "brave/browser/component_updater/extension_installer_traits.h"
#include "chrome/browser/browser_process.h"
#include "components/component_updater/component_updater_service.h"
#include "components/update_client/crx_update_item.h"
#include "native_mate/dictionary.h"


void ComponentsUI::OnDemandUpdate(
    component_updater::ComponentUpdateService* cus,
    const std::string& component_id) {
  g_browser_process->component_updater()
    ->GetOnDemandUpdater().OnDemandUpdate(component_id);
}

bool
ComponentsUI::GetComponentDetails(const std::string& id,
                                  update_client::CrxUpdateItem* item) const {
  return g_browser_process->component_updater()
    ->GetComponentDetails(id, item);
}

namespace brave {

namespace api {

ComponentUpdater::ComponentUpdater(v8::Isolate* isolate) {
  Init(isolate);
}

ComponentUpdater::~ComponentUpdater() {
}

void ComponentUpdater::OnEvent(Events event, const std::string& id) {
  update_client::CrxUpdateItem item;
  GetComponentDetails(id, &item);
  switch (event) {
    case Events::COMPONENT_CHECKING_FOR_UPDATES:
      Emit("component-checking-for-updates", id);
      break;
    case Events::COMPONENT_WAIT:
      Emit("component-wait", id);
      break;
    case Events::COMPONENT_UPDATE_FOUND:
      Emit("component-update-found", id);
      break;
    case Events::COMPONENT_UPDATE_READY:
      Emit("component-update-ready", id);
      break;
    case Events::COMPONENT_UPDATED:
      Emit("component-update-updated", id, item.component.version.GetString());
      break;
    case Events::COMPONENT_NOT_UPDATED:
      Emit("component-not-updated", id);
      break;
    case Events::COMPONENT_UPDATE_DOWNLOADING:
      Emit("component-update-downloading", id);
      break;
  }
}

void ComponentUpdater::RegisterComponentForUpdate(
    const std::string& public_key,
    const base::Closure& registered_callback,
    const ReadyCallback& ready_callback) {
  brave::RegisterExtension(
      g_browser_process->component_updater(),
      public_key, registered_callback, ready_callback);
}

void ComponentUpdater::OnComponentRegistered(const std::string& extension_id) {
  Emit("component-registered", extension_id);
}

void ComponentUpdater::OnComponentReady(
    const std::string& extension_id,
    const base::FilePath& install_dir) {
  Emit("component-ready", extension_id,
    std::string(install_dir.value().begin(), install_dir.value().end()));
}

void ComponentUpdater::RegisterComponent(const std::string& extension_id) {
  static bool registeredObserver = false;
  if (!registeredObserver) {
    g_browser_process->component_updater()->AddObserver(this);
    registeredObserver = true;
  }
  base::Closure registered_callback =
    base::Bind(&ComponentUpdater::OnComponentRegistered,
               base::Unretained(this), extension_id);
  ReadyCallback ready_callback =
    base::Bind(&ComponentUpdater::OnComponentReady,
               base::Unretained(this), extension_id);
  if (extension_id == kOnePasswordId) {
    RegisterComponentForUpdate(
        kOnePasswordPublicKeyStr, registered_callback, ready_callback);
  } else if (extension_id == kDashlaneId) {
    RegisterComponentForUpdate(
        kDashlanePublicKeyStr, registered_callback, ready_callback);
  } else if (extension_id == kLastPassId) {
    RegisterComponentForUpdate(
        kLastPassPublicKeyStr, registered_callback, ready_callback);
  } else if (extension_id == kPDFJSId) {
    RegisterComponentForUpdate(
        kPDFJSPublicKeyStr, registered_callback, ready_callback);
  }
}

void ComponentUpdater::CheckNow(const std::string& extension_id) {
  OnDemandUpdate(g_browser_process->component_updater(), extension_id);
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
