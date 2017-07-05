// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_APP_H_
#define ATOM_BROWSER_API_ATOM_API_APP_H_

#include <memory>
#include <string>

#include "atom/browser/api/event_emitter.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/browser_observer.h"
#include "atom/common/native_mate_converters/callback.h"
#include "chrome/browser/process_singleton.h"
#include "content/public/browser/gpu_data_manager_observer.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "native_mate/handle.h"
#include "net/base/completion_callback.h"
#include "net/base/network_change_notifier.h"

namespace base {
class FilePath;
}

namespace mate {
class Arguments;
}  // namespace mate

namespace atom {

#if defined(OS_WIN)
enum class JumpListResult : int;
#endif

namespace api {

class App : public AtomBrowserClient::Delegate,
            public mate::EventEmitter<App>,
            public BrowserObserver,
            public net::NetworkChangeNotifier::MaxBandwidthObserver,
            public content::GpuDataManagerObserver,
            public content::NotificationObserver {
 public:
  static mate::Handle<App> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // Called when window with disposition needs to be created.
  void OnCreateWindow(const GURL& target_url,
                      const std::string& frame_name,
                      WindowOpenDisposition disposition,
                      int render_process_id,
                      int render_frame_id);

 protected:
  explicit App(v8::Isolate* isolate);
  ~App() override;

  // BrowserObserver:
  void OnBeforeQuit(bool* prevent_default) override;
  void OnWillQuit(bool* prevent_default) override;
  void OnWindowAllClosed() override;
  void OnQuit() override;
  void OnOpenFile(bool* prevent_default, const std::string& file_path) override;
  void OnOpenURL(const std::string& url) override;
  void OnActivate(bool has_visible_windows) override;
  void OnWillFinishLaunching() override;
  void OnFinishLaunching(const base::DictionaryValue& launch_info) override;
  void OnLogin(LoginHandler* login_handler,
               const base::DictionaryValue& request_details) override;
  void OnAccessibilitySupportChanged() override;
#if defined(OS_MACOSX)
  void OnContinueUserActivity(
      bool* prevent_default,
      const std::string& type,
      const base::DictionaryValue& user_info) override;
#endif

  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      content::ResourceType resource_type,
      bool overridable,
      bool strict_enforcement,
      bool expired_previous_decision,
      const base::Callback<void(content::CertificateRequestResultType)>&
        callback) override;
  void SelectClientCertificate(
      content::WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      net::ClientCertIdentityList client_certs,
      std::unique_ptr<content::ClientCertificateDelegate> delegate) override;

  // net::NetworkChangeNotifier::MaxBandwidthObserver:
  void OnMaxBandwidthChanged(
      double max_bandwidth_mbps,
      net::NetworkChangeNotifier::ConnectionType type) override;

  // content::GpuDataManagerObserver:
  void OnGpuProcessCrashed(base::TerminationStatus exit_code) override;

  void Observe(
    int type, const content::NotificationSource& source,
    const content::NotificationDetails& details) override;

 private:
  // Get/Set the pre-defined path in PathService.
  base::FilePath GetPath(mate::Arguments* args, const std::string& name);
  void SetPath(mate::Arguments* args,
               const std::string& name,
               const base::FilePath& path);

  void SetDesktopName(const std::string& desktop_name);
  void SetLocale(std::string);
  std::string GetLocale();
  bool MakeSingleInstance(
      const ProcessSingleton::NotificationCallback& callback);
  void ReleaseSingleInstance();
  bool Relaunch(mate::Arguments* args);
  void DisableHardwareAcceleration(mate::Arguments* args);
  bool IsAccessibilitySupportEnabled();
  void SendMemoryPressureAlert();
  void PostMessage(int worker_id,
                  v8::Local<v8::Value> message,
                  mate::Arguments* args);
  void StartWorker(mate::Arguments* args);
  void StopWorker(mate::Arguments* args);

#if defined(OS_WIN)
  // Get the current Jump List settings.
  v8::Local<v8::Value> GetJumpListSettings();

  // Set or remove a custom Jump List for the application.
  JumpListResult SetJumpList(v8::Local<v8::Value> val, mate::Arguments* args);
#endif  // defined(OS_WIN)

  content::NotificationRegistrar registrar_;

  std::unique_ptr<ProcessSingleton> process_singleton_;

  DISALLOW_COPY_AND_ASSIGN(App);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_APP_H_
