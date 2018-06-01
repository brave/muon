// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/tor/tor_launcher_factory.h"

#include <algorithm>
#include <limits>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_launcher_utils.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"
#include "url/third_party/mozilla/url_parse.h"

namespace brave {

using content::BrowserThread;

TorLauncherFactory::TorLauncherFactory(
  const base::FilePath::StringType& path, const std::string& proxy)
  : path_(path) {
  if (proxy.length()) {
    url::Parsed url;
    url::ParseStandardURL(
      proxy.c_str(),
      std::min(proxy.size(),
               static_cast<size_t>(std::numeric_limits<int>::max())),
      &url);
    if (url.host.is_valid()) {
      host_ =
        std::string(proxy.begin() + url.host.begin,
                    proxy.begin() + url.host.begin + url.host.len);
    }
    if (url.port.is_valid()) {
      port_ =
        std::string(proxy.begin() + url.port.begin,
                    proxy.begin() + url.port.begin + url.port.len);
    }
  }

  content::ServiceManagerConnection::GetForProcess()->GetConnector()
    ->BindInterface(tor::mojom::kTorServiceName,
                    &tor_launcher_);

  tor_launcher_.set_connection_error_handler(
    base::BindOnce(&TorLauncherFactory::OnTorLauncherCrashed,
                   base::Unretained(this)));
}

TorLauncherFactory::~TorLauncherFactory() {}

void TorLauncherFactory::LaunchTorProcess() {
  content::GetProcessLauncherTaskRunner()->PostTask(
    FROM_HERE, base::Bind(&TorLauncherFactory::LaunchOnLauncherThread,
                          base::Unretained(this)));
}

void TorLauncherFactory::RelaunchTorProcess() {
  content::GetProcessLauncherTaskRunner()->PostTask(
    FROM_HERE, base::Bind(&TorLauncherFactory::RelaunchOnLauncherThread,
                          base::Unretained(this)));
}

void TorLauncherFactory::SetLauncherCallback(
    const TorLauncherCallback& callback) {
  callback_ = callback;
}

void TorLauncherFactory::LaunchOnLauncherThread() {
  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  if (!user_data_dir.empty()) {
    tor_data_path_ = user_data_dir.Append(FILE_PATH_LITERAL("tor"))
      .Append(FILE_PATH_LITERAL("data"));
    base::CreateDirectory(tor_data_path_);
    tor_watch_path_ = user_data_dir.Append(FILE_PATH_LITERAL("tor"))
      .Append(FILE_PATH_LITERAL("watch"));
    base::CreateDirectory(tor_watch_path_);
  }
  tor_launcher_->Launch(base::FilePath(path_), host_, port_, tor_data_path_,
                        tor_watch_path_,
                        base::Bind(&TorLauncherFactory::OnTorLaunched,
                                   base::Unretained(this)));
  tor_launcher_->SetCrashHandler(base::Bind(
                        &TorLauncherFactory::OnTorCrashed,
                        base::Unretained(this)));
}

void TorLauncherFactory::RelaunchOnLauncherThread() {
  tor_launcher_->Relaunch(base::FilePath(path_), host_, port_, tor_data_path_,
                        tor_watch_path_,
                        base::Bind(&TorLauncherFactory::OnTorLaunched,
                                   base::Unretained(this)));
}

void TorLauncherFactory::OnTorLauncherCrashed() {
  LOG(ERROR) << "Tor Launcher Crashed";
}

void TorLauncherFactory::OnTorCrashed(int64_t pid) {
  LOG(ERROR) << "Tor Process(" << pid << ") Crashed";
  if (callback_)
    callback_.Run(TorProcessState::CRASHED, pid);
}

void TorLauncherFactory::OnTorLaunched(bool result, int64_t pid) {
  tor_pid_ = pid;
  if (!result) {
    LOG(ERROR) << "Tor Launching Failed(" << pid <<")";
  }
  if (callback_) {
    if (result)
      callback_.Run(TorProcessState::LAUNCH_SUCCEEDED, pid);
    else
      callback_.Run(TorProcessState::LAUNCH_FAILED, pid);
  }
}

}  // namespace brave
