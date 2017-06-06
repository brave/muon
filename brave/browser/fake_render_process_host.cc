// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/fake_render_process_host.h"

#include "atom/browser/extensions/atom_extension_system.h"
#include "base/memory/shared_memory.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/common/child_process_host_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host_observer.h"
#include "content/public/renderer/render_thread.h"
#include "extensions/browser/extension_message_filter.h"
#include "extensions/browser/info_map.h"
#include "media/audio/audio_output_controller.h"

using content::BrowserThread;
using content::NotificationService;
using content::RenderThread;
using content::RenderThreadObserver;
using content::ResourceDispatcherDelegate;
using content::ServiceManagerConnection;
using extensions::ExtensionMessageFilter;
using extensions::ExtensionSystem;
using extensions::InfoMap;
using extensions::ProcessMap;

class FakeRenderThread : public RenderThread {
 public:
  explicit FakeRenderThread(FakeRenderProcessHost* host) : host_(host) {}
  IPC::SyncChannel* GetChannel() override { return nullptr; }
  std::string GetLocale() { return std::string(); }
  IPC::SyncMessageFilter* GetSyncMessageFilter() override { return nullptr; }
  scoped_refptr<base::SingleThreadTaskRunner> GetIOTaskRunner() override {
    return nullptr;
  }
  void AddRoute(int32_t routing_id, IPC::Listener* listener) override {}
  void RemoveRoute(int32_t routing_id) override {}
  int GenerateRoutingID() override { return MSG_ROUTING_NONE; }
  void AddFilter(IPC::MessageFilter* filter) override {}
  void RemoveFilter(IPC::MessageFilter* filter) override {}
  void AddObserver(RenderThreadObserver* observer) override {}
  void RemoveObserver(RenderThreadObserver* observer) override {}
  void SetResourceDispatcherDelegate(
      ResourceDispatcherDelegate* delegate) override {}
  std::unique_ptr<base::SharedMemory> HostAllocateSharedMemoryBuffer(
      size_t buffer_size) override {
    std::unique_ptr<base::SharedMemory> shared_buf(new base::SharedMemory);
    if (!shared_buf->CreateAnonymous(buffer_size)) {
      NOTREACHED() << "Cannot map shared memory buffer";
      return std::unique_ptr<base::SharedMemory>();
    }

    return std::unique_ptr<base::SharedMemory>(shared_buf.release());
  }
  cc::SharedBitmapManager* GetSharedBitmapManager() override { return nullptr; }
  void RegisterExtension(v8::Extension* extension) override {}
  void ScheduleIdleHandler(int64_t initial_delay_ms) override {}
  void IdleHandler() override {}
  int64_t GetIdleNotificationDelayInMs() const override { return 0; }
  void SetIdleNotificationDelayInMs(
      int64_t idle_notification_delay_in_ms) override {}
  int PostTaskToAllWebWorkers(const base::Closure& closure) override {
    return 0;
  }
  bool ResolveProxy(const GURL& url, std::string* proxy_list) override {
    return false;
  }
  base::WaitableEvent* GetShutdownEvent() override { return nullptr; }
  int32_t GetClientId() override { return 1; }
  scoped_refptr<base::SingleThreadTaskRunner> GetTimerTaskRunner() override {
    return base::ThreadTaskRunnerHandle::Get();
  }
  scoped_refptr<base::SingleThreadTaskRunner> GetLoadingTaskRunner() override {
    return base::ThreadTaskRunnerHandle::Get();
  }
  bool Send(IPC::Message* msg) override {
    return host_->Send(msg);
  }
  void RecordAction(const base::UserMetricsAction& action) override {}
  void RecordComputedAction(const std::string& action) override {}
  ServiceManagerConnection* GetServiceManagerConnection() override {
    return nullptr;
  }
  service_manager::InterfaceRegistry* GetInterfaceRegistry() override {
    return nullptr;
  }
  service_manager::Connector* GetConnector() override {
    return nullptr;
  }
  void SetFieldTrialGroup(const std::string& trial_name,
                                  const std::string& group_name) override {}

 private:
  FakeRenderProcessHost* host_;

  DISALLOW_COPY_AND_ASSIGN(FakeRenderThread);
};

FakeRenderProcessHost::FakeRenderProcessHost(BrowserContext* browser_context) :
    id_(content::ChildProcessHostImpl::GenerateChildProcessUniqueId()),
    browser_context_(browser_context),
    init_time_(base::TimeTicks::Now()) {
  if (!RenderThread::Get()) {
    new FakeRenderThread(this);
  }
  content::ChildProcessSecurityPolicyImpl::GetInstance()->Add(GetID());
  content::RenderProcessHostImpl::RegisterHost(GetID(), this);
  AddFilter(new ExtensionMessageFilter(GetID(), browser_context));
}

FakeRenderProcessHost::~FakeRenderProcessHost() {
  for (auto& observer : observers_) {
    observer.RenderProcessExited(
        this, base::TERMINATION_STATUS_NORMAL_TERMINATION, 0);
  }

  for (auto& observer : observers_)
    observer.RenderProcessHostDestroyed(this);
    NotificationService::current()->Notify(
        content::NOTIFICATION_RENDERER_PROCESS_TERMINATED,
        content::Source<RenderProcessHost>(this),
        NotificationService::NoDetails());

  for (std::vector<BrowserMessageFilter*>::const_iterator it = filters_.begin();
       it != filters_.end(); ++it) {
    (*it)->OnDestruct();
  }
  content::RenderProcessHostImpl::UnregisterHost(GetID());
}

void FakeRenderProcessHost::AddExtension(
    const extensions::Extension* extension) {
  ProcessMap::Get(browser_context_)->Insert(extension->id(), GetID(), -1);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&InfoMap::RegisterExtensionProcess,
         ExtensionSystem::Get(browser_context_)->info_map(), extension->id(),
            GetID(), -1));
}

void FakeRenderProcessHost::RemoveExtension(
    const extensions::Extension* extension) {
  ProcessMap::Get(browser_context_)->Remove(extension->id(), GetID(), -1);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&InfoMap::UnregisterExtensionProcess,
         ExtensionSystem::Get(browser_context_)->info_map(), extension->id(),
            GetID(), -1));
}

void FakeRenderProcessHost::AddObserver(RenderProcessHostObserver* observer) {
  observers_.AddObserver(observer);
}

void FakeRenderProcessHost::RemoveObserver(
    RenderProcessHostObserver* observer) {
  observers_.RemoveObserver(observer);
}

void FakeRenderProcessHost::Cleanup() {
  BrowserThread::DeleteOnUIThread::Destruct(this);
}

BrowserContext* FakeRenderProcessHost::GetBrowserContext() const {
  return browser_context_;
}

void FakeRenderProcessHost::AddFilter(BrowserMessageFilter* filter) {
  filters_.push_back(filter);
}

bool FakeRenderProcessHost::Send(IPC::Message* message) {
  for (std::vector<BrowserMessageFilter*>::const_iterator it = filters_.begin();
       it != filters_.end(); ++it) {
    if ((*it)->OnMessageReceived(*message)) {
      return true;
    }
  }
  return false;
}

StoragePartition* FakeRenderProcessHost::GetStoragePartition() const {
  return BrowserContext::GetDefaultStoragePartition(GetBrowserContext());
}

base::ProcessHandle FakeRenderProcessHost::GetHandle() const {
  return base::GetCurrentProcessHandle();
}

#if BUILDFLAG(ENABLE_WEBRTC)
FakeRenderProcessHost::WebRtcStopRtpDumpCallback
FakeRenderProcessHost::StartRtpDump(
    bool incoming,
    bool outgoing,
    const WebRtcRtpPacketCallback& packet_callback) {
  return WebRtcStopRtpDumpCallback();
}
#endif

void FakeRenderProcessHost::GetAudioOutputControllers(
    const GetAudioOutputControllersCallback& callback) const {
  RenderProcessHost::AudioOutputControllerList controllers;
  callback.Run(controllers);
}

