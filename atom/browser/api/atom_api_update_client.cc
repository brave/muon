// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_update_client.h"

#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/browser/brave_content_browser_client.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/dictionary.h"
#include "extensions/browser/updater/update_service.h"
#include "extensions/browser/updater/update_service_factory.h"

namespace atom {

namespace api {

UpdateClient::UpdateClient(v8::Isolate* isolate,
                 content::BrowserContext* browser_context)
      : browser_context_(browser_context) {
  Init(isolate);
}

UpdateClient::~UpdateClient() {
}

void UpdateClient::Install(const std::string& extension_id) {
  LOG(INFO) << "Install extension ID!" << extension_id.c_str();
}

void UpdateClient::Update(const std::string& extension_id) {
  LOG(INFO) << "Update!";

  // For now we only support one
  std::vector<std::string> extensions = {extension_id};
  extensions::UpdateService* updateService =
      extensions::UpdateServiceFactory::GetForBrowserContext(browser_context_);
  updateService->StartUpdateCheck(extensions);
}

// static
mate::Handle<UpdateClient> UpdateClient::Create(
    v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  return mate::CreateHandle(isolate, new UpdateClient(isolate, browser_context));
}

// static
void UpdateClient::BuildPrototype(v8::Isolate* isolate,
                              v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "UpdateClient"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
    .SetMethod("install", &UpdateClient::Install)
    .SetMethod("update", &UpdateClient::Update);
}

}  // namespace api

}  // namespace atom
