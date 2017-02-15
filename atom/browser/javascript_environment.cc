// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/javascript_environment.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/i18n/icu_util.h"
#include "base/message_loop/message_loop.h"
#include "content/public/common/content_switches.h"
#include "gin/array_buffer.h"
#include "gin/v8_initializer.h"

#include "extensions/common/features/feature.h"
#include "extensions/renderer/logging_native_handler.h"
#include "extensions/renderer/module_system.h"
#include "extensions/renderer/native_handler.h"
#include "extensions/renderer/resource_bundle_source_map.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/utils_native_handler.h"
#include "ui/base/resource/resource_bundle.h"

using extensions::Feature;
using extensions::LoggingNativeHandler;
using extensions::ModuleSystem;
using extensions::NativeHandler;
using extensions::ResourceBundleSourceMap;
using extensions::ScriptContext;
using extensions::UtilsNativeHandler;
using ui::ResourceBundle;

namespace atom {

namespace {

std::vector<base::FilePath> GetModuleSearchPaths() {
  // std::vector<base::FilePath> search_paths(2);
  // PathService::Get(base::DIR_SOURCE_ROOT, &search_paths[0]);
  // PathService::Get(base::DIR_EXE, &search_paths[1]);
  // search_paths[1] = search_paths[1].AppendASCII("gen");
  // return search_paths;
  return std::vector<base::FilePath>();
}

}  // namespace

RunnerDelegate::RunnerDelegate() :
    gin::ModuleRunnerDelegate(GetModuleSearchPaths()) {
}

RunnerDelegate::~RunnerDelegate() {}

v8::Local<v8::ObjectTemplate> RunnerDelegate::GetGlobalTemplate(
      gin::ShellRunner* runner,
      v8::Isolate* isolate) {
  return v8::Local<v8::ObjectTemplate>();
}

JavascriptEnvironment::JavascriptEnvironment()
    : initialized_(Initialize()),
      isolate_(isolate_holder_.isolate()),
      locker_(isolate_),
      runner_(new gin::ShellRunner(&delegate_, isolate_)),
      scope_(runner_.get()),
      source_map_(&ResourceBundle::GetSharedInstance()) {
  script_context_.reset(new ScriptContext(context(),
                                   nullptr,  // WebFrame
                                   nullptr,  // Extension
                                   Feature::BLESSED_EXTENSION_CONTEXT,
                                   nullptr,  // Effective Extension
                                   Feature::BLESSED_EXTENSION_CONTEXT));
  script_context_->v8_context()->Enter();
  {
    std::unique_ptr<extensions::ModuleSystem> module_system(
        new ModuleSystem(script_context_.get(), &source_map_));
    script_context_->set_module_system(std::move(module_system));
  }
}

JavascriptEnvironment::~JavascriptEnvironment() {
  if (script_context_.get() && script_context_->is_valid()) {
    script_context_->v8_context()->Exit();
    script_context_->Invalidate();
  }
}

void JavascriptEnvironment::OnMessageLoopCreated() {
  isolate_holder_.AddRunMicrotasksObserver();
}

void JavascriptEnvironment::OnMessageLoopDestroying() {
  isolate_holder_.RemoveRunMicrotasksObserver();
}

bool JavascriptEnvironment::Initialize() {
  auto cmd = base::CommandLine::ForCurrentProcess();

  // --js-flags.
  std::string js_flags = cmd->GetSwitchValueASCII(switches::kJavaScriptFlags);
  if (!js_flags.empty())
    v8::V8::SetFlagsFromString(js_flags.c_str(), js_flags.size());

  gin::IsolateHolder::Initialize(gin::IsolateHolder::kNonStrictMode,
                                 gin::IsolateHolder::kStableV8Extras,
                                 gin::ArrayBufferAllocator::SharedInstance());
  return true;
}

}  // namespace atom
