// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "browser/net_log.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "content/public/common/content_switches.h"
#include "net/log/file_net_log_observer.h"
#include "net/log/net_log_util.h"
#include "services/network/public/cpp/network_switches.h"


namespace brightray {

namespace {

std::unique_ptr<base::DictionaryValue> GetConstants() {
  std::unique_ptr<base::DictionaryValue> constants = net::GetNetConstants();

  // Adding client information to constants dictionary.
  auto client_info = base::MakeUnique<base::DictionaryValue>();
  client_info->SetString(
      "command_line",
      base::CommandLine::ForCurrentProcess()->GetCommandLineString());

  constants->Set("clientInfo", std::move(client_info));
  return constants;
}

}  // namespace

NetLog::NetLog() {
}

NetLog::~NetLog() {
}

void NetLog::StartLogging() {
  auto command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(network::switches::kLogNetLog))
    return;

  base::FilePath log_path = command_line->GetSwitchValuePath(network::switches::kLogNetLog);
  net::NetLogCaptureMode capture_mode = net::NetLogCaptureMode::Default();

  file_net_log_observer_ = net::FileNetLogObserver::CreateUnbounded(
      log_path, GetConstants());
  file_net_log_observer_->StartObserving(this, capture_mode);
}

}  // namespace brightray
