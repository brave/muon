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
  BrowserThread::PostTask(
    BrowserThread::PROCESS_LAUNCHER, FROM_HERE,
    base::Bind(&TorLauncherFactory::LaunchInLauncherThread,
               base::Unretained(this)));
}

void TorLauncherFactory::LaunchInLauncherThread() {
  base::FilePath user_data_dir;
  base::FilePath tor_data_path;
  PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  if (!user_data_dir.empty()) {
    tor_data_path = user_data_dir.Append(FILE_PATH_LITERAL("tor"))
      .Append(FILE_PATH_LITERAL("data"));
    base::CreateDirectory(tor_data_path);
  }
  tor_launcher_->Launch(base::FilePath(path_), host_, port_, tor_data_path,
                        base::Bind(&TorLauncherFactory::OnTorLaunched,
                                   base::Unretained(this)));
  tor_launcher_->SetCrashHandler(base::Bind(
                        &TorLauncherFactory::OnTorCrashed,
                        base::Unretained(this)));
}

void TorLauncherFactory::OnTorLauncherCrashed() {
  LOG(ERROR) << "Tor Launcher Crashed";
}

void TorLauncherFactory::OnTorCrashed(int64_t pid) {
  LOG(ERROR) << "Tor Process(" << pid << ") Crashed";
}

void TorLauncherFactory::OnTorLaunched(bool result) {
  if (!result)
    LOG(ERROR) << "Tor Launching Failed";
}

}  // namespace brave
