// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMMON_WORKERS_V8_WORKER_THREAD_H_
#define BRAVE_COMMON_WORKERS_V8_WORKER_THREAD_H_

#include <memory>
#include <string>

#include "base/memory/memory_pressure_listener.h"
#include "base/threading/thread.h"

namespace atom {
class JavascriptEnvironment;
namespace api {
class App;
}
}

namespace brave {

class V8WorkerThread : public base::Thread {
 public:
  explicit V8WorkerThread(const std::string& name,
      const std::string& module_name, atom::api::App* app);
  ~V8WorkerThread() override;

  static V8WorkerThread* current();
  static void Shutdown();

  void Init() override;
  void Run(base::RunLoop* run_loop) override;
  void CleanUp() override;

  atom::api::App* app() const { return app_; }
  atom::JavascriptEnvironment* env() const { return js_env_.get(); }
  const std::string& module_name() const { return module_name_; }

 private:
  void LoadModule();
  void OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  const std::string module_name_;
  atom::api::App* app_;
  std::unique_ptr<atom::JavascriptEnvironment> js_env_;
  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;
};

}  // namespace brave

#endif  // BRAVE_COMMON_WORKERS_V8_WORKER_THREAD_H_
