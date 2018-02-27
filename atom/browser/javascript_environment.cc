// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/javascript_environment.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/i18n/icu_util.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brave/common/extensions/crash_reporter_bindings.h"
#include "brave/common/extensions/crypto_bindings.h"
#include "brave/common/extensions/file_bindings.h"
#include "brave/common/extensions/path_bindings.h"
#include "brave/common/extensions/shared_memory_bindings.h"
#include "brave/common/extensions/url_bindings.h"
#include "content/public/common/content_switches.h"
#include "extensions/common/features/feature.h"
#include "extensions/renderer/logging_native_handler.h"
#include "extensions/renderer/module_system.h"
#include "extensions/renderer/native_handler.h"
#include "extensions/renderer/resource_bundle_source_map.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/utils_native_handler.h"
#include "gin/array_buffer.h"
#include "gin/modules/console.h"
#include "gin/object_template_builder.h"
#include "gin/v8_initializer.h"
#include "ui/base/resource/resource_bundle.h"


using extensions::Feature;
using extensions::ModuleSystem;
using extensions::ScriptContext;
using gin::ObjectTemplateBuilder;

namespace atom {

namespace {

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

}  // namespace

JavascriptEnvironment::JavascriptEnvironment()
    : initialized_(Initialize()),
      isolate_holder_(
        new gin::IsolateHolder(base::ThreadTaskRunnerHandle::Get())),
      isolate_(isolate_holder_->isolate()),
      isolate_scope_(isolate_),
      locker_(isolate_),
      handle_scope_(isolate_),
      context_holder_(new gin::ContextHolder(isolate_)),
      source_map_(GetModuleSearchPaths()) {
  v8::Local<v8::ObjectTemplate> templ = ObjectTemplateBuilder(isolate_).Build();

  v8::Local<v8::Context> ctx =
      v8::Context::New(isolate_,
          g_v8_extension_configurator.Get().GetConfiguration(),
          templ);
  context_holder_->SetContext(ctx);
  context()->Enter();

  script_context_.reset(new ScriptContext(context(),
                                   nullptr,  // WebFrame
                                   nullptr,  // Extension
                                   Feature::SERVICE_WORKER_CONTEXT,
                                   nullptr,  // Effective Extension
                                   Feature::SERVICE_WORKER_CONTEXT));
  {
    std::unique_ptr<ModuleSystem> module_system(
        new ModuleSystem(script_context_.get(), &source_map_));
    script_context_->SetModuleSystem(std::move(module_system));
    script_context_->module_system()->RegisterNativeHandler(
      "path", std::unique_ptr<extensions::NativeHandler>(
          new brave::PathBindings(script_context_.get(), &source_map_)));
  }

  v8::Local<v8::Object> global = context()->Global();
  v8::Local<v8::Object> muon = v8::Object::New(isolate_);
  global->Set(v8::String::NewFromUtf8(isolate_, "muon"), muon);

  v8::Local<v8::Object> shared_memory =
      brave::SharedMemoryBindings::API(script_context_.get());
  muon->Set(v8::String::NewFromUtf8(isolate_, "shared_memory"), shared_memory);
  muon->Set(v8::String::NewFromUtf8(isolate_, "sharedMemory"), shared_memory);

  v8::Local<v8::Object> file =
      brave::FileBindings::API(script_context_.get());
  muon->Set(v8::String::NewFromUtf8(isolate_, "file"), file);

  v8::Local<v8::Object> url =
      brave::URLBindings::API(script_context_.get());
  muon->Set(v8::String::NewFromUtf8(isolate_, "url"), url);

  v8::Local<v8::Object> crash_reporter =
      brave::CrashReporterBindings::API(script_context_.get());
  muon->Set(v8::String::NewFromUtf8(isolate_, "crashReporter"), crash_reporter);

  v8::Local<v8::Object> crypto =
      brave::CryptoBindings::API(script_context_.get());
  muon->Set(v8::String::NewFromUtf8(isolate_, "crypto"), crypto);
}

JavascriptEnvironment::~JavascriptEnvironment() {
  context()->Exit();
  if (script_context_.get() && script_context_->is_valid()) {
    script_context_->Invalidate();
  }
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
