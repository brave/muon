// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/javascript_environment.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "atom/browser/extensions/atom_extension_system.h"
#include "atom/grit/atom_resources.h"  // NOLINT: This file is generated
#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/i18n/icu_util.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram_macros.h"
#include "base/path_service.h"
#include "base/threading/thread_local.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brave/browser/fake_render_process_host.h"
#include "brave/common/extensions/file_bindings.h"
#include "brave/common/extensions/path_bindings.h"
#include "brave/common/extensions/shared_memory_bindings.h"
#include "brave/common/extensions/url_bindings.h"
#include "brave/common/workers/v8_worker_thread.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "content/child/blink_platform_impl.h"
#include "content/child/worker_thread_registry.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/common/content_switches.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_function_dispatcher.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/features/feature.h"
#include "extensions/common/url_pattern.h"
#include "extensions/grit/extensions_renderer_resources.h"
#include "extensions/renderer/event_bindings.h"
#include "extensions/renderer/id_generator_custom_bindings.h"
#include "extensions/renderer/js_extension_bindings_system.h"
#include "extensions/renderer/logging_native_handler.h"
#include "extensions/renderer/module_system.h"
#include "extensions/renderer/native_handler.h"
#include "extensions/renderer/process_info_native_handler.h"
#include "extensions/renderer/request_sender.h"
#include "extensions/renderer/resource_bundle_source_map.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/script_context_set.h"
#include "extensions/renderer/send_request_natives.h"
#include "extensions/renderer/user_gestures_native_handler.h"
#include "extensions/renderer/utils_native_handler.h"
#include "extensions/renderer/v8_context_native_handler.h"
#include "extensions/renderer/v8_schema_registry.h"
#include "gin/array_buffer.h"
#include "gin/modules/console.h"
#include "gin/modules/module_registry.h"
#include "gin/object_template_builder.h"
#include "gin/v8_initializer.h"
#include "third_party/WebKit/public/platform/Platform.h"
#include "ui/base/resource/resource_bundle.h"

using base::SingleThreadTaskRunner;
using brave::V8WorkerThread;
using content::BrowserMessageFilter;
using content::BrowserThread;
using extensions::Extension;
using extensions::ExtensionRegistry;
using extensions::ExtensionSystem;
using extensions::Feature;
using extensions::ModuleSystem;
using extensions::ScriptContext;
using extensions::EventBindings;
using extensions::EventRouter;
using extensions::ExtensionFunctionDispatcher;
using extensions::IdGeneratorCustomBindings;
using extensions::LoggingNativeHandler;
using extensions::JsExtensionBindingsSystem;
using extensions::NativeHandler;
using extensions::ProcessInfoNativeHandler;
using extensions::RequestSender;
using extensions::ScriptContext;
using extensions::ScriptContextSet;
using extensions::SendRequestNatives;
using extensions::UnloadedExtensionInfo;
using extensions::UtilsNativeHandler;
using extensions::V8ContextNativeHandler;
using extensions::V8SchemaRegistry;
using gin::ModuleRegistry;
using gin::ObjectTemplateBuilder;

namespace atom {

namespace {

static const char kInternalExtensionId[] = "maonmaonmaonmaonmaonmaonmaonmaon";

class LocalAPIActivityLogger : public extensions::ObjectBackedNativeHandler {
 public:
  explicit LocalAPIActivityLogger(ScriptContext* context)
      : ObjectBackedNativeHandler(context) {
    RouteFunction("LogEvent", base::Bind(&LocalAPIActivityLogger::LogEvent,
                                       base::Unretained(this)));
    RouteFunction("LogAPICall", base::Bind(&LocalAPIActivityLogger::LogAPICall,
                                         base::Unretained(this)));
  }
  void LogAPICall(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // TODO(bridiver)
  }
  void LogEvent(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // TODO(bridiver)
  }
};

std::vector<std::pair<const char*, int>> GetJsResources() {
  // Libraries.
  std::vector<std::pair<const char*, int>> resources = {
    {extensions::kEventBindings, IDR_EVENT_BINDINGS_JS},
    {"json_schema", IDR_JSON_SCHEMA_JS},
    {"lastError", IDR_LAST_ERROR_JS},
    {extensions::kSchemaUtils, IDR_SCHEMA_UTILS_JS},
    {"sendRequest", IDR_SEND_REQUEST_JS},
    {"uncaught_exception_handler", IDR_UNCAUGHT_EXCEPTION_HANDLER_JS},
    {"utils", IDR_UTILS_JS},

    // Custom bindings.
    {"i18n", IDR_I18N_CUSTOM_BINDINGS_JS},
    {"permissions", IDR_PERMISSIONS_CUSTOM_BINDINGS_JS},
    {"binding", IDR_BINDING_JS},

    // Custom types sources.
    {"StorageArea", IDR_STORAGE_AREA_JS},
  };

  return resources;
}

base::FilePath GetResourcesDir() {
  auto command_line = base::CommandLine::ForCurrentProcess();
  base::FilePath exec_path(command_line->GetProgram());
  base::PathService::Get(base::FILE_EXE, &exec_path);

  base::FilePath resources_path =
#if defined(OS_MACOSX)
      exec_path.DirName().DirName().Append(FILE_PATH_LITERAL("Resources"));
#else
      exec_path.DirName().Append(FILE_PATH_LITERAL("resources"));
#endif
  return resources_path;
}

std::vector<base::FilePath> GetModuleSearchPaths() {
  std::vector<base::FilePath> search_paths;

  auto command_line = base::CommandLine::ForCurrentProcess();
  base::FilePath source_root = command_line->GetSwitchValuePath("source-root");
  if (source_root.empty()) {
    source_root = GetResourcesDir().Append(FILE_PATH_LITERAL("app.asar"));
  }

  bool path_exists = base::PathExists(source_root);
  if (!path_exists) {
    base::PathService::Get(base::DIR_SOURCE_ROOT, &source_root);
    path_exists = base::PathExists(source_root);
  }

  if (path_exists) {
    search_paths.push_back(source_root);
    search_paths.push_back(source_root.Append(FILE_PATH_LITERAL("app")));
    search_paths.push_back(source_root
        .Append(FILE_PATH_LITERAL("app"))
        .Append(FILE_PATH_LITERAL("node_modules")));
    search_paths.push_back(source_root
        .Append(FILE_PATH_LITERAL("node_modules")));
  }

  search_paths.push_back(
    GetResourcesDir().Append(FILE_PATH_LITERAL("electron.asar")));

  return search_paths;
}

class LocalRequestSender : public RequestSender {
 public:
  LocalRequestSender(base::WeakPtr<ExtensionFunctionDispatcher> dispatcher,
                    int render_process_id,
                    int worker_thread_id) :
      dispatcher_(dispatcher),
      render_process_id_(render_process_id),
      worker_thread_id_(worker_thread_id),
      service_worker_version_id_(0) {}

  void SendRequest(content::RenderFrame* render_frame,
                    bool for_io_thread,
                    ExtensionHostMsg_Request_Params& params) override {
    params.worker_thread_id = worker_thread_id_;
    params.service_worker_version_id = service_worker_version_id_;

    if (for_io_thread) {
      BrowserThread::PostTask(
          BrowserThread::IO, FROM_HERE,
          base::Bind(&ExtensionFunctionDispatcher::Dispatch,
              dispatcher_,
              params, nullptr, render_process_id_));
    } else {
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&ExtensionFunctionDispatcher::Dispatch,
              dispatcher_,
              params, nullptr, render_process_id_));
    }
  }

 private:
  base::WeakPtr<ExtensionFunctionDispatcher> dispatcher_;
  const int render_process_id_;
  const int worker_thread_id_;
  const int64_t service_worker_version_id_;

  DISALLOW_COPY_AND_ASSIGN(LocalRequestSender);
};

class V8ExtensionConfigurator {
 public:
  V8ExtensionConfigurator()
      : safe_builtins_(extensions::SafeBuiltins::CreateV8Extension()),
        names_(1, safe_builtins_->name()),
        configuration_(
            new v8::ExtensionConfiguration(static_cast<int>(names_.size()),
                                           names_.data())) {
    v8::RegisterExtension(safe_builtins_.get());
  }

  v8::ExtensionConfiguration* GetConfiguration() {
    return configuration_.get();
  }

 private:
  std::unique_ptr<v8::Extension> safe_builtins_;
  std::vector<const char*> names_;
  std::unique_ptr<v8::ExtensionConfiguration> configuration_;
};

base::LazyInstance<V8ExtensionConfigurator>::Leaky g_v8_extension_configurator =
    LAZY_INSTANCE_INITIALIZER;

// Keep the JavascriptEnvironment in a TLS slot so it is impossible to access
// incorrectly from the wrong thread.
static base::LazyInstance<base::ThreadLocalPointer<JavascriptEnvironment> >::Leaky
    lazy_tls = LAZY_INSTANCE_INITIALIZER;

}  // namespace

class LocalRequestSenderMessageFilter : public BrowserMessageFilter {
 public:
  explicit LocalRequestSenderMessageFilter(
                              scoped_refptr<SingleThreadTaskRunner> handle) :
      BrowserMessageFilter(ExtensionMsgStart),
      handle_(handle) {}

  bool OnMessageReceived(const IPC::Message& message) override {
    bool handled = false;

    IPC_BEGIN_MESSAGE_MAP(LocalRequestSenderMessageFilter, message)
    IPC_MESSAGE_HANDLER(ExtensionMsg_DispatchEvent, OnDispatchEvent)
    IPC_MESSAGE_HANDLER(ExtensionMsg_ResponseWorker, OnResponseWorker)
    IPC_MESSAGE_UNHANDLED(handled = false)
    IPC_END_MESSAGE_MAP()
    return handled;
  }

  void OnDestruct() const override {
    delete this;
  }

 private:
  void OnResponseWorker(int worker_thread_id,
                        int request_id,
                        bool succeeded,
                        const base::ListValue& response,
                        const std::string& error) {
    handle_->PostTask(FROM_HERE,
        base::Bind(JavascriptEnvironment::HandleResponse,
            request_id, succeeded,
            base::Passed(response.CreateDeepCopy()), error));
  }

  void OnDispatchEvent(
      const ExtensionMsg_DispatchEvent_Params& params,
      const base::ListValue& event_args) {
    handle_->PostTask(FROM_HERE,
        base::Bind(JavascriptEnvironment::DispatchEvent,
                    params.extension_id,
                    params.event_name,
                    event_args,
                    params.filtering_info));
  }

  scoped_refptr<SingleThreadTaskRunner> handle_;

  DISALLOW_COPY_AND_ASSIGN(LocalRequestSenderMessageFilter);
};

// static
JavascriptEnvironment* JavascriptEnvironment::Get() {
  return lazy_tls.Pointer()->Get();
}

scoped_refptr<const Extension> CreateInternalExtension(
    BrowserContext* browser_context) {
  base::DictionaryValue manifest;
  manifest.SetString("name", std::string("Internal Extension"));
  manifest.SetString("version", "1.0");

  std::unique_ptr<base::ListValue> permissions(new base::ListValue());
  permissions->AppendString("unlimitedStorage");
  permissions->AppendString("storage");
  manifest.Set("permissions", permissions.release());

  std::string error;
  auto extension = Extension::Create(base::FilePath(),
                        extensions::Manifest::COMPONENT,
                        manifest,
                        Extension::WAS_INSTALLED_BY_OEM,
                        kInternalExtensionId,
                        &error);
  // map chrome://brave/* to internal extension id
  extension->AddWebExtentPattern(
      URLPattern(URLPattern::SCHEME_CHROMEUI, "chrome://brave/*"));
  return extension;
}

JavascriptEnvironment::JavascriptEnvironment()
    : initialized_(Initialize()),
      isolate_holder_(
        new gin::IsolateHolder(base::ThreadTaskRunnerHandle::Get())),
      isolate_(isolate_holder_->isolate()),
      isolate_scope_(isolate_),
      locker_(isolate_),
      handle_scope_(isolate_),
      context_holder_(new gin::ContextHolder(isolate_)),
      source_map_(GetModuleSearchPaths()),
      browser_context_(ProfileManager::GetActiveUserProfile()),
      fake_render_process_host_(new FakeRenderProcessHost(browser_context_)),
      dispatcher_(new ExtensionFunctionDispatcher(browser_context_)),
      v8_schema_registry_(new V8SchemaRegistry) {
  lazy_tls.Pointer()->Set(this);
  v8::Local<v8::ObjectTemplate> templ = ObjectTemplateBuilder(isolate_).Build();
  ModuleRegistry::RegisterGlobals(isolate_, templ);

  v8::Local<v8::Context> ctx =
      v8::Context::New(isolate_,
          g_v8_extension_configurator.Get().GetConfiguration(),
          templ);
  context_holder_->SetContext(ctx);
  context()->Enter();

  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    blink_platform_impl_.reset(new content::BlinkPlatformImpl);
    blink::Platform::Initialize(blink_platform_impl_.get());
  }

  PopulateSourceMap();

  ModuleRegistry* registry = ModuleRegistry::From(context());
  registry->AddBuiltinModule(isolate(),
      gin::Console::kModuleName,
      gin::Console::GetModule(isolate()));

  fake_render_process_host_->AddFilter(
      new LocalRequestSenderMessageFilter(base::ThreadTaskRunnerHandle::Get()));

  int worker_thread_id_ = -1;
  if (V8WorkerThread::current()) {
    worker_thread_id_ = V8WorkerThread::current()->GetThreadId();
  } else {
    worker_thread_id_ = BrowserThread::UI;
  }

  bindings_system_.reset(new JsExtensionBindingsSystem(
      source_map_.GetResourceBundleSourceMap(),
      base::MakeUnique<LocalRequestSender>(
          dispatcher_->AsWeakPtr(),
          fake_render_process_host_->GetID(), worker_thread_id_)));

  auto extension_registry =
      extensions::ExtensionRegistry::Get(browser_context_);
  const Extension* internal_extension =
      extension_registry->GetExtensionById(kInternalExtensionId,
                                            ExtensionRegistry::ENABLED);
  extension_registry->AddObserver(this);
  if (!internal_extension) {
    auto extension_service =
        ExtensionSystem::Get(browser_context_)->extension_service();
    internal_extension = extension_service->AddExtension(
        CreateInternalExtension(browser_context_).get());
  }
  DCHECK(internal_extension);
  SetExtension(internal_extension);
}

JavascriptEnvironment::~JavascriptEnvironment() {
  auto extension_registry =
        extensions::ExtensionRegistry::Get(browser_context_);
  extension_registry->RemoveObserver(this);
  lazy_tls.Pointer()->Set(NULL);
  context()->Exit();
  if (script_context_.get() && script_context_->is_valid()) {
    DestroyScriptContext();
    fake_render_process_host_->Cleanup();
  }
}

void JavascriptEnvironment::DestroyScriptContext() {
  if (script_context_.get() && script_context_->is_valid()) {
    bindings_system_->WillReleaseScriptContext(script_context_.get());
    const Extension* extension = script_context_->extension();
    if (extension) {
      fake_render_process_host_->RemoveExtension(extension);
    }
    script_context_->Invalidate();
    script_context_.reset();
  }
}

void JavascriptEnvironment::SetExtension(const Extension* extension) {
  if (!script_context_.get() ||
      extension != script_context_->extension()) {
    DestroyScriptContext();
    script_context_.reset(CreateScriptContext(extension));
  }
}

void JavascriptEnvironment::OnExtensionUnloaded(content::BrowserContext* browser_context,
   const Extension* extension,
   UnloadedExtensionInfo::Reason reason) {
  if (browser_context == browser_context_ &&
      extension && script_context_->extension() &&
      extension->id() == script_context_->extension()->id()) {
    DCHECK(extension->id() != kInternalExtensionId);

    auto extension_registry =
        extensions::ExtensionRegistry::Get(browser_context_);
    const Extension* internal_extension =
        extension_registry->GetExtensionById(kInternalExtensionId,
                                              ExtensionRegistry::ENABLED);
    DCHECK(internal_extension);
    SetExtension(internal_extension);
  }
}

ScriptContext* JavascriptEnvironment::CreateScriptContext(const Extension* extension) {
  fake_render_process_host_->AddExtension(extension);

  ScriptContext* script_context = new ScriptContext(context(),
                                   nullptr,  // WebFrame
                                   extension,  // Extension
                                   Feature::SERVICE_WORKER_CONTEXT,
                                   extension,  // Effective Extension
                                   Feature::SERVICE_WORKER_CONTEXT);

  if (extension) {
    script_context->set_url(
        GURL("chrome://brave/worker/" +
            extension->id() + "/" + std::to_string(worker_thread_id_)));
  }
  std::unique_ptr<ModuleSystem> module_system(
      new ModuleSystem(script_context, &source_map_));
  script_context->set_module_system(std::move(module_system));

  DidCreateScriptContext(script_context);
  return script_context;
}

void JavascriptEnvironment::DidCreateScriptContext(
    ScriptContext* script_context) {
  v8::Local<v8::Object> global = context()->Global();
  v8::Local<v8::Object> muon = v8::Object::New(isolate_);
  global->Set(v8::String::NewFromUtf8(isolate_, "muon"), muon);

  // empty stub for apis
  v8::Local<v8::Object> navigator = v8::Object::New(isolate_);
  global->Set(v8::String::NewFromUtf8(isolate_, "navigator"), navigator);

  v8::Local<v8::Object> shared_memory =
      brave::SharedMemoryBindings::API(script_context);
  muon->Set(v8::String::NewFromUtf8(isolate_, "shared_memory"), shared_memory);

  v8::Local<v8::Object> file =
      brave::FileBindings::API(script_context);
  muon->Set(v8::String::NewFromUtf8(isolate_, "file"), file);

  v8::Local<v8::Object> url =
      brave::URLBindings::API(script_context);
  muon->Set(v8::String::NewFromUtf8(isolate_, "url"), url);

  // Enable natives in startup.
  extensions::ModuleSystem* module_system = script_context->module_system();
  extensions::ModuleSystem::NativesEnabledScope
      natives_enabled_scope(module_system);

  RegisterNativeHandlers(module_system,
                         script_context,
                         bindings_system_->GetRequestSender(),
                         v8_schema_registry_.get());

  bindings_system_->DidCreateScriptContext(script_context);
  bindings_system_->UpdateBindingsForContext(script_context);
}

void JavascriptEnvironment::PopulateSourceMap() {
  const std::vector<std::pair<const char*, int>> resources = GetJsResources();
  for (const auto& resource : resources) {
    source_map()->RegisterSource(resource.first, resource.second);
  }
}

EventRouter* JavascriptEnvironment::GetEventRouter() {
  DCHECK(browser_context_);
  return EventRouter::Get(browser_context_);
}

void JavascriptEnvironment::HandleResponse(int request_id,
                               bool success,
                               const std::unique_ptr<base::ListValue>& response,
                               const std::string& error) {
  auto env = JavascriptEnvironment::Get();
  if (env) {
    env->bindings_system_->HandleResponse(
        request_id, success, *response, error);
  }
}

void JavascriptEnvironment::DispatchEvent(
    const std::string& extension_id,
    const std::string& event_name,
    const base::ListValue& event_args,
    const base::DictionaryValue& filtering_info) {
  auto env = JavascriptEnvironment::Get();
  if (env) {
    env->bindings_system_->DispatchEventInContext(
        event_name, &event_args, &filtering_info, env->script_context());
  }
}

void JavascriptEnvironment::RegisterNativeHandlers(
                                      extensions::ModuleSystem* module_system,
                                      ScriptContext* script_context,
                                      RequestSender* request_sender,
                                      V8SchemaRegistry* v8_schema_registry) {
  module_system->RegisterNativeHandler(
      "path",
      std::unique_ptr<extensions::NativeHandler>(
          new brave::PathBindings(script_context, &source_map_)));
  module_system->RegisterNativeHandler(
      "logging",
      std::unique_ptr<NativeHandler>(
          new LoggingNativeHandler(script_context)));
  module_system->RegisterNativeHandler("schema_registry",
                                       v8_schema_registry->AsNativeHandler());
  module_system->RegisterNativeHandler(
      "v8_script_context",
      std::unique_ptr<NativeHandler>(
          new V8ContextNativeHandler(script_context)));
  module_system->RegisterNativeHandler(
      "sendRequest", std::unique_ptr<NativeHandler>(
          new SendRequestNatives(request_sender, script_context)));
  module_system->RegisterNativeHandler(
      "event_natives",
      std::unique_ptr<NativeHandler>(new EventBindings(script_context)));
  module_system->RegisterNativeHandler(
      "utils", std::unique_ptr<NativeHandler>(
          new UtilsNativeHandler(script_context)));
  module_system->RegisterNativeHandler(
      "id_generator",
      std::unique_ptr<NativeHandler>(
          new IdGeneratorCustomBindings(script_context)));
  module_system->RegisterNativeHandler(
      "process",
      std::unique_ptr<NativeHandler>(new ProcessInfoNativeHandler(
          script_context,
          script_context->extension()
              ? script_context->extension()->id()
              : std::string(),
          script_context->GetContextTypeDescription(),
          browser_context_->IsOffTheRecord(),
          script_context->extension()
              ? script_context->extension()->location() ==
                  extensions::Manifest::COMPONENT
              : false,
          script_context->extension()
              ? script_context->extension()->manifest_version()
              : 2,
          false /* sendRequest disabled */)));
  module_system->RegisterNativeHandler(
      "activityLogger", std::unique_ptr<NativeHandler>(
                            new LocalAPIActivityLogger(script_context)));
}

void JavascriptEnvironment::OnMessageLoopCreated() {
  isolate_holder_->AddRunMicrotasksObserver();
}

void JavascriptEnvironment::OnMessageLoopDestroying() {
  isolate_holder_->RemoveRunMicrotasksObserver();
}

bool JavascriptEnvironment::Initialize() {
  auto cmd = base::CommandLine::ForCurrentProcess();

  // --js-flags.
  std::string js_flags = cmd->GetSwitchValueASCII(switches::kJavaScriptFlags);
  if (!js_flags.empty())
    v8::V8::SetFlagsFromString(js_flags.c_str(), js_flags.size());

  #ifdef V8_USE_EXTERNAL_STARTUP_DATA
    gin::V8Initializer::LoadV8Snapshot();
    gin::V8Initializer::LoadV8Natives();
  #endif

  gin::IsolateHolder::Initialize(gin::IsolateHolder::kNonStrictMode,
                                 gin::IsolateHolder::kStableV8Extras,
                                 gin::ArrayBufferAllocator::SharedInstance());


  return true;
}

}  // namespace atom
