// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_JAVASCRIPT_ENVIRONMENT_H_
#define ATOM_BROWSER_JAVASCRIPT_ENVIRONMENT_H_

#include <memory>

#include "base/macros.h"
#include "brave/common/extensions/asar_source_map.h"
#include "extensions/renderer/script_context.h"
#include "gin/public/context_holder.h"
#include "gin/public/isolate_holder.h"
#include "v8/include/v8.h"

namespace extensions {
class ModuleSystem;
}

namespace atom {

class JavascriptEnvironment {
 public:
  JavascriptEnvironment();
  ~JavascriptEnvironment();

  void OnMessageLoopCreated();
  void OnMessageLoopDestroying();

  v8::Isolate* isolate() const { return isolate_; }
  extensions::ScriptContext* script_context() const {
    return script_context_.get();
  }
  extensions::ModuleSystem* module_system() const {
    return script_context_->module_system(); }
  v8::Local<v8::Context> context() const {
    return context_holder_->context();
  }
  const brave::AsarSourceMap& source_map() const {
    return source_map_;
  }

 private:
  bool Initialize();

  bool initialized_;
  std::unique_ptr<gin::IsolateHolder> isolate_holder_;
  v8::Isolate* isolate_;
  v8::Isolate::Scope isolate_scope_;
  v8::Locker locker_;
  v8::HandleScope handle_scope_;
  std::unique_ptr<gin::ContextHolder> context_holder_;
  brave::AsarSourceMap source_map_;
  std::unique_ptr<extensions::ScriptContext> script_context_;

  DISALLOW_COPY_AND_ASSIGN(JavascriptEnvironment);
};

}  // namespace atom

#endif  // ATOM_BROWSER_JAVASCRIPT_ENVIRONMENT_H_
