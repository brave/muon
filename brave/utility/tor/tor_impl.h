// Copyright (c) 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UTILITY_TOR_TOR_IMPL_H_
#define BRAVE_UTILITY_TOR_TOR_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/process/process.h"
#include "brave/common/tor/tor.mojom.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace brave {

class TorLauncherImpl : public tor::mojom::TorLauncher {
 public:
  explicit TorLauncherImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~TorLauncherImpl() override;

  // tor::mojom::TorLauncher
  void Launch(const base::FilePath& tor_bin,
              const std::string& tor_host, const std::string& tor_port,
              const base::FilePath& tor_data_dir,
              LaunchCallback callback) override;
  void SetCrashHandler(SetCrashHandlerCallback callback) override;

 private:
  void MonitorChild();

  SetCrashHandlerCallback crash_handler_callback_;
  std::unique_ptr<base::Thread> child_monitor_thread_;
  base::Process tor_process_;
  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(TorLauncherImpl);
};

}  // namespace brave

#endif  // BRAVE_UTILITY_TOR_TOR_IMPL_H_
