// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/common/workers/v8_worker_thread.h"

#include "atom/browser/api/atom_api_app.h"
#include "atom/browser/javascript_environment.h"
#include "base/lazy_instance.h"
#include "base/run_loop.h"
#include "base/threading/thread_local.h"
#include "brave/common/workers/worker_bindings.h"
#include "content/public/browser/browser_thread.h"
#include "content/renderer/worker_thread_registry.h"

using content::BrowserThread;
using extensions::ModuleSystem;

namespace brave {

namespace {

base::LazyInstance<base::ThreadLocalPointer<V8WorkerThread>>::Leaky worker =
      LAZY_INSTANCE_INITIALIZER;

void NotifyStart(atom::api::App* app, int worker_id) {
  app->Emit("worker-start", worker_id);
}

void NotifyStop(atom::api::App* app, int worker_id) {
  app->Emit("worker-stop", worker_id);
}

void NotifyError(atom::api::App* app, int worker_id, std::string error) {
  app->Emit("worker-onerror", worker_id, error);
}

void Kill(V8WorkerThread* worker) {
  delete worker;
}

}  // namespace

V8WorkerThread::V8WorkerThread(const std::string& name,
                              const std::string& module_name,
                              atom::api::App* app) :
    base::Thread(name),
    module_name_(module_name),
    app_(app) {
}

V8WorkerThread::~V8WorkerThread() {
  Stop();
}

// static
V8WorkerThread* V8WorkerThread::current() {
  return worker.Get().Get();
}

// static
void V8WorkerThread::Shutdown() {
  V8WorkerThread* instance = current();
  if (!instance)
    return;

  worker.Get().Set(nullptr);

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      base::Bind(&NotifyStop,
                  base::Unretained(instance->app()),
                  instance->GetThreadId()));

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(&Kill, base::Unretained(instance)));
}

void V8WorkerThread::Init() {
  worker.Get().Set(this);

  js_env_.reset(new atom::JavascriptEnvironment());

  env()->module_system()->RegisterNativeHandler(
      "worker", std::unique_ptr<extensions::NativeHandler>(
          new WorkerBindings(env()->script_context(), this)));

  memory_pressure_listener_.reset(new base::MemoryPressureListener(
      base::Bind(&V8WorkerThread::OnMemoryPressure,
        base::Unretained(this))));
}

void V8WorkerThread::Run(base::RunLoop* run_loop) {
  base::ThreadRestrictions::SetIOAllowed(true);
  content::WorkerThreadRegistry::Instance()->DidStartCurrentWorkerThread();
  env()->OnMessageLoopCreated();
  LoadModule();
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      base::Bind(&NotifyStart,
                  base::Unretained(app()),
                  GetThreadId()));
  Thread::Run(run_loop);
}

// Called just after the message loop ends
void V8WorkerThread::CleanUp() {
  content::WorkerThreadRegistry::Instance()->WillStopCurrentWorkerThread();
  memory_pressure_listener_.reset();
  env()->OnMessageLoopDestroying();
  js_env_.reset();
  V8WorkerThread::Shutdown();
}

void V8WorkerThread::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  env()->isolate()->LowMemoryNotification();
}

void V8WorkerThread::LoadModule() {
  if (!env()->source_map().Contains(module_name_)) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
        base::Bind(&NotifyError,
                    base::Unretained(app()),
                    GetThreadId(),
                    "No source for require(" + module_name_ + ")"));
    base::RunLoop::QuitCurrentDeprecated();
    return;
  }

  ModuleSystem::NativesEnabledScope natives_enabled(env()->module_system());
  env()->module_system()->Require(module_name_);
}

}  // namespace brave
