// Copyright (c) 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/net/proxy/proxy_config_service_tor.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "chrome/common/chrome_paths.h"
#include "crypto/random.h"
#include "net/proxy/proxy_service.h"
#include "net/url_request/url_request_context.h"
#include "services/service_manager/public/cpp/connector.h"

namespace net {

using content::BrowserThread;
const int kTorPasswordLength = 16;

static bool g_tor_launched = false;

ProxyConfigServiceTor::ProxyConfigServiceTor(
  const base::FilePath::StringType& tor_path,
  const std::string& tor_proxy) {
    if (tor_path.length() && tor_proxy.length()) {
      tor_path_ = tor_path;
      url::Parsed url;
      url::ParseStandardURL(
        tor_proxy.c_str(),
        std::min(tor_proxy.size(),
                 static_cast<size_t>(std::numeric_limits<int>::max())),
        &url);
      if (url.scheme.is_valid()) {
        scheme_ =
          std::string(tor_proxy.begin() + url.scheme.begin,
                      tor_proxy.begin() + url.scheme.begin + url.scheme.len);
      }
      if (url.host.is_valid()) {
        host_ =
          std::string(tor_proxy.begin() + url.host.begin,
                      tor_proxy.begin() + url.host.begin + url.host.len);
      }
      if (url.port.is_valid()) {
        port_ =
          std::string(tor_proxy.begin() + url.port.begin,
                      tor_proxy.begin() + url.port.begin + url.port.len);
      }

      if (!g_tor_launched) {
        content::ServiceManagerConnection::GetForProcess()
          ->GetConnector()
          ->BindInterface(tor::mojom::kTorServiceName,
                          &tor_launcher_);

        tor_launcher_.set_connection_error_handler(
          base::BindOnce(&ProxyConfigServiceTor::OnTorLauncherCrashed,
                         base::Unretained(this)));

        BrowserThread::PostTask(
          BrowserThread::PROCESS_LAUNCHER, FROM_HERE,
          base::Bind(&ProxyConfigServiceTor::LaunchTorProcess,
                     base::Unretained(this)));
      }
    }
    config_.proxy_rules().ParseFromString(std::string(scheme_ + "://" + host_
      + ":" + port_));
}

void ProxyConfigServiceTor::OnTorCrashed(int64_t pid) {
  LOG(ERROR) << "Tor Process(" << pid << ") Crashed";
  g_tor_launched = false;
}

ProxyConfigServiceTor::~ProxyConfigServiceTor() {}

void ProxyConfigServiceTor::LaunchTorProcess() {
  base::FilePath user_data_dir;
  base::FilePath tor_data_path;
  PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  if (!user_data_dir.empty()) {
    tor_data_path = user_data_dir.Append(FILE_PATH_LITERAL("tor"))
      .Append(FILE_PATH_LITERAL("data"));
    base::CreateDirectory(tor_data_path);
  }
  tor_launcher_->Launch(base::FilePath(tor_path_), host_, port_, tor_data_path,
                        base::Bind(&ProxyConfigServiceTor::OnTorLaunched,
                                   base::Unretained(this)));
  tor_launcher_->SetCrashHandler(base::Bind(
                        &ProxyConfigServiceTor::OnTorCrashed,
                        base::Unretained(this)));
}

void ProxyConfigServiceTor::TorSetProxy(
    scoped_refptr<brightray::URLRequestContextGetter>
      url_request_context_getter,
    const std::string tor_proxy,
    const base::FilePath::StringType& tor_path,
    const bool isolated_storage,
    const base::FilePath partition_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!url_request_context_getter || !isolated_storage)
    return;
  auto proxy_service = url_request_context_getter->GetURLRequestContext()->
    proxy_service();
  // Notice CreateRequestContextForStoragePartition will only be called once
  // per partition_path so there is no need to cache password per origin
  std::string origin = partition_path.DirName().BaseName().AsUTF8Unsafe();
  std::unique_ptr<net::ProxyConfigServiceTor>
    config(new ProxyConfigServiceTor(tor_path, tor_proxy));
  config->SetUsername(origin);
  proxy_service->ResetConfigService(std::move(config));
}

void ProxyConfigServiceTor::OnTorLauncherCrashed() {
  LOG(ERROR) << "Tor Launcher Crashed";
}

void ProxyConfigServiceTor::OnTorLaunched(bool result) {
  if (!result)
    LOG(ERROR) << "Tor Launching Failed";
  else
    g_tor_launched = true;
}

ProxyConfigServiceTor::ConfigAvailability
    ProxyConfigServiceTor::GetLatestProxyConfig(ProxyConfig* config) {
  if (scheme_ != kSocksProxy || host_.empty() || port_.empty())
    return CONFIG_UNSET;
  std::string password = GenerateNewPassword();
  std::string url = std::string(scheme_ + "://" + username_ + ":" + password +
    "@" + host_ + ":" + port_);
  config_.proxy_rules().ParseFromString(url);
  *config = config_;
  return CONFIG_VALID;
}

bool ProxyConfigServiceTor::SetUsername(const std::string& username) {
  if (username.empty())
    return false;
  username_ = username;
  return true;
}


std::string ProxyConfigServiceTor::GenerateNewPassword() {
  std::vector<uint8_t> password(kTorPasswordLength);
  crypto::RandBytes(password.data(), password.size());
  return base::HexEncode(password.data(), password.size());
}

}  // namespace net
