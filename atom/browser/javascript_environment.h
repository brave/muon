// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_JAVASCRIPT_ENVIRONMENT_H_
#define ATOM_BROWSER_JAVASCRIPT_ENVIRONMENT_H_

#include <memory>

#include "base/macros.h"
#include "extensions/renderer/resource_bundle_source_map.h"
#include "extensions/renderer/script_context.h"
#include "gin/modules/module_runner_delegate.h"
#include "gin/public/context_holder.h"
#include "gin/public/isolate_holder.h"
#include "gin/shell_runner.h"
#include "v8/include/v8.h"

namespace atom {

class RunnerDelegate : public gin::ModuleRunnerDelegate {
 public:
  RunnerDelegate();
  ~RunnerDelegate() override;

 private:
  v8::Local<v8::ObjectTemplate> GetGlobalTemplate(
      gin::ShellRunner* runner,
      v8::Isolate* isolate) override;
  DISALLOW_COPY_AND_ASSIGN(RunnerDelegate);
};

class JavascriptEnvironment {
 public:
  JavascriptEnvironment();
  ~JavascriptEnvironment();

  void OnMessageLoopCreated();
  void OnMessageLoopDestroying();

  v8::Isolate* isolate() const { return isolate_; }
  v8::Local<v8::Context> context() const {
    return runner_->GetContextHolder()->context();
  }

 private:
  bool Initialize();

  bool initialized_;
  gin::IsolateHolder isolate_holder_;
  v8::Isolate* isolate_;
  v8::Locker locker_;
  RunnerDelegate delegate_;
  std::unique_ptr<gin::ShellRunner> runner_;
  gin::Runner::Scope scope_;
  extensions::ResourceBundleSourceMap source_map_;
  std::unique_ptr<extensions::ScriptContext> script_context_;

  DISALLOW_COPY_AND_ASSIGN(JavascriptEnvironment);
};

}  // namespace atom

#endif  // ATOM_BROWSER_JAVASCRIPT_ENVIRONMENT_H_
