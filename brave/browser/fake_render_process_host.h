// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_FAKE_RENDER_PROCESS_HOST_H_
#define BRAVE_BROWSER_FAKE_RENDER_PROCESS_HOST_H_

#include <memory>
#include <string>
#include <vector>

#include "content/public/browser/render_process_host.h"

using content::BrowserContext;
using content::BrowserMessageFilter;
using content::GlobalRequestID;
using content::RenderProcessHostObserver;
using content::StoragePartition;

namespace extensions {
class Extension;
}

class FakeRenderProcessHost : public content::RenderProcessHost {
 public:
  explicit FakeRenderProcessHost(BrowserContext* browser_context);
  ~FakeRenderProcessHost();

  void AddExtension(const extensions::Extension* extension);
  void RemoveExtension(const extensions::Extension* extension);

  // RenderProcessHost implementation
  BrowserContext* GetBrowserContext() const override;
  int GetID() const override { return id_; }
  bool Send(IPC::Message* message) override;
  bool Init() override { return true; }
  void EnableSendQueue() override {}
  int GetNextRoutingID() override { return MSG_ROUTING_NONE; }
  void AddRoute(int32_t routing_id, IPC::Listener* listener) override {}
  void RemoveRoute(int32_t routing_id) override {}
  void AddObserver(RenderProcessHostObserver* observer) override;
  void RemoveObserver(RenderProcessHostObserver* observer) override;
  void ShutdownForBadMessage(CrashReportMode crash_report_mode) override {}
  void WidgetRestored() override {}
  void WidgetHidden() override {}
  int VisibleWidgetCount() const override { return 0; }
  bool IsForGuestsOnly() const override { return false; }
  void OnAudioStreamAdded() override {}
  void OnAudioStreamRemoved() override {}
  StoragePartition* GetStoragePartition() const override;
  bool Shutdown(int exit_code, bool wait) override { return false; }
  bool FastShutdownIfPossible() override { return false; }
  bool FastShutdownStarted() const override { return false; }
  base::ProcessHandle GetHandle() const override;
  bool IsReady() const override { return true; }
  bool HasConnection() const override { return true; }
  void SetIgnoreInputEvents(bool ignore_input_events) override {}
  bool IgnoreInputEvents() const override { return true; }
  void Cleanup() override;
  void AddPendingView() override {}
  void RemovePendingView() override {}
  void SetSuddenTerminationAllowed(bool allowed) override {}
  bool SuddenTerminationAllowed() const override {}
  bool InSameStoragePartition(StoragePartition* partition) const override {
    return partition == GetStoragePartition();
  }
  IPC::ChannelProxy* GetChannel() override { return nullptr; }
  void AddFilter(BrowserMessageFilter* filter) override;
  bool FastShutdownForPageCount(size_t count) override { return false; }
  base::TimeDelta GetChildProcessIdleTime() const override {
    return init_time_ - init_time_;
  }
  void FilterURL(bool empty_allowed, GURL* url) override {}
#if BUILDFLAG(ENABLE_WEBRTC)
  void EnableAudioDebugRecordings(const base::FilePath& file) override {}
  void DisableAudioDebugRecordings() override {}
  bool StartWebRTCEventLog(const base::FilePath& file_path) override {
    return false;
  }
  bool StopWebRTCEventLog() override {
    return false;
  }
  void SetWebRtcLogMessageCallback(
      base::Callback<void(const std::string&)> callback) override {}
  void ClearWebRtcLogMessageCallback() override {}
  WebRtcStopRtpDumpCallback StartRtpDump(
      bool incoming,
      bool outgoing,
      const WebRtcRtpPacketCallback& packet_callback) override;
#endif
  void GetAudioOutputControllers(
      const GetAudioOutputControllersCallback& callback) const override;
  void ResumeDeferredNavigation(const GlobalRequestID& request_id) override {}
  service_manager::InterfaceProvider* GetRemoteInterfaces() override {
    return nullptr;
  }
  std::unique_ptr<base::SharedPersistentMemoryAllocator>
  TakeMetricsAllocator() override { return nullptr; }
  const base::TimeTicks& GetInitTimeForNavigationMetrics() const override {
    return init_time_;
  }
  bool IsProcessBackgrounded() const override { return true; }
  size_t GetWorkerRefCount() const override { return 0; }
  void IncrementServiceWorkerRefCount() override {}
  void DecrementServiceWorkerRefCount() override {}
  void IncrementSharedWorkerRefCount() override {}
  void DecrementSharedWorkerRefCount() override {}
  void ForceReleaseWorkerRefCounts() override {}
  bool IsWorkerRefCountDisabled() override { return true; }
  void PurgeAndSuspend() override {}
  void Resume() override {}
  content::mojom::Renderer* GetRendererInterface() override { return nullptr; }
  void SetIsNeverSuitableForReuse() override {}
  bool MayReuseHost() override { return true; }

  // IPC::Listener via RenderProcessHost.
  bool OnMessageReceived(const IPC::Message& msg) override { return false; }
  void OnChannelConnected(int32_t peer_pid) override {}

 private:
  const int id_;
  BrowserContext* browser_context_;  // not owned
  const base::TimeTicks init_time_;
  std::vector<BrowserMessageFilter*> filters_;

  // The observers watching our lifetime.
  base::ObserverList<RenderProcessHostObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(FakeRenderProcessHost);
};

#endif  // BRAVE_BROWSER_FAKE_RENDER_PROCESS_HOST_H_
