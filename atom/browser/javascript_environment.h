// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_JAVASCRIPT_ENVIRONMENT_H_
#define ATOM_BROWSER_JAVASCRIPT_ENVIRONMENT_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "brave/common/extensions/asar_source_map.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/renderer/script_context.h"
#include "gin/modules/module_runner_delegate.h"
#include "gin/public/context_holder.h"
#include "gin/public/isolate_holder.h"
#include "v8/include/v8.h"

class FakeRenderProcessHost;

namespace base {
class ListValue;
}

namespace content {
class BlinkPlatformImpl;
class BrowserContext;
}

namespace extensions {
class EventRouter;
class Extension;
class ExtensionBindingsSystem;
class ExtensionFunctionDispatcher;
class ModuleSystem;
class ScriptContext;
class RequestSender;
class V8SchemaRegistry;
}

namespace atom {

class LocalRequestSenderMessageFilter;

class JavascriptEnvironment : public extensions::ExtensionRegistryObserver {
 public:
  static JavascriptEnvironment* Get();

  JavascriptEnvironment();
  ~JavascriptEnvironment();

  void OnMessageLoopCreated();
  void OnMessageLoopDestroying();

  void SetExtension(const extensions::Extension* extension);

  v8::Isolate* isolate() const { return isolate_; }
  extensions::ScriptContext* script_context() {
    return script_context_.get();
  }
  extensions::ModuleSystem* module_system() {
    return script_context_->module_system(); }
  v8::Local<v8::Context> context() {
    return context_holder_->context();
  }
  brave::AsarSourceMap* source_map() {
    return &source_map_;
  }

 private:
  friend class LocalRequestSenderMessageFilter;

  bool Initialize();

  extensions::EventRouter* GetEventRouter();
  void DidCreateScriptContext(extensions::ScriptContext* context);
  void PopulateSourceMap();
  void RegisterNativeHandlers(extensions::ModuleSystem* module_system,
                              extensions::ScriptContext* context,
                              extensions::RequestSender* request_sender,
                              extensions::V8SchemaRegistry* v8_schema_registry);

  static void HandleResponse(int request_id,
                             bool success,
                             const std::unique_ptr<base::ListValue>& response,
                             const std::string& error);
  static void DispatchEvent(
      const std::string& extension_id,
      const std::string& event_name,
      const base::ListValue& event_args,
      const base::DictionaryValue& filtering_info);

  void DestroyScriptContext();
  extensions::ScriptContext* CreateScriptContext(
      const extensions::Extension* extension);
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
     const extensions::Extension* extension,
     extensions::UnloadedExtensionInfo::Reason reason) override;


  bool initialized_;
  std::unique_ptr<gin::IsolateHolder> isolate_holder_;
  v8::Isolate* isolate_;
  v8::Isolate::Scope isolate_scope_;
  v8::Locker locker_;
  v8::HandleScope handle_scope_;
  std::unique_ptr<gin::ContextHolder> context_holder_;
  brave::AsarSourceMap source_map_;
  content::BrowserContext* browser_context_;  // not owned
  FakeRenderProcessHost* fake_render_process_host_;
  std::unique_ptr<extensions::ExtensionFunctionDispatcher> dispatcher_;
  std::unique_ptr<extensions::ExtensionBindingsSystem> bindings_system_;
  std::unique_ptr<extensions::V8SchemaRegistry> v8_schema_registry_;
  int worker_thread_id_;

  std::unique_ptr<extensions::ScriptContext> script_context_;
  std::unique_ptr<content::BlinkPlatformImpl> blink_platform_impl_;

  DISALLOW_COPY_AND_ASSIGN(JavascriptEnvironment);
};

}  // namespace atom

#endif  // ATOM_BROWSER_JAVASCRIPT_ENVIRONMENT_H_
