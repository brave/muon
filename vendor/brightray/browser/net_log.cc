// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "browser/net_log.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/values.h"
#include "content/public/common/content_switches.h"
#include "net/log/net_log_util.h"

namespace brightray {

namespace {

std::unique_ptr<base::DictionaryValue> GetConstants() {
  std::unique_ptr<base::DictionaryValue> constants = net::GetNetConstants();

  // Adding client information to constants dictionary.
  auto* client_info = new base::DictionaryValue();
  client_info->SetString(
      "command_line",
      base::CommandLine::ForCurrentProcess()->GetCommandLineString());

  constants->Set("clientInfo", client_info);
  return constants;
}

}  // namespace

NetLog::NetLog() {
}

NetLog::~NetLog() {
}

void NetLog::StartLogging(net::URLRequestContext* url_request_context) {
  auto command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(switches::kLogNetLog))
    return;
  // WIP
  net::NetLogCaptureMode capture_mode;
  write_to_file_observer_.StartObserving(this, capture_mode);
}

}  // namespace brightray
