// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_TOR_TOR_LAUNCHER_FACTORY_H_
#define BRAVE_BROWSER_TOR_TOR_LAUNCHER_FACTORY_H_

#include <string>

#include "base/callback.h"
#include "brave/common/tor/tor.mojom.h"

namespace brave {

class TorLauncherFactory {
 public:
  TorLauncherFactory(const base::FilePath::StringType& path,
                      const std::string& proxy);
  ~TorLauncherFactory();

  enum class TorProcessState {
    LAUNCH_SUCCEEDED,
    LAUNCH_FAILED,
    CRASHED
  };

  using TorLauncherCallback = base::Callback<void(TorProcessState, int64_t)>;

  void LaunchTorProcess();
  void RelaunchTorProcess();
  void SetLauncherCallback(const TorLauncherCallback& callback);
  int64_t GetTorPid() const { return tor_pid_; }

 private:
  void LaunchOnLauncherThread();
  void RelaunchOnLauncherThread();

  void OnTorLauncherCrashed();
  void OnTorCrashed(int64_t pid);
  void OnTorLaunched(bool result, int64_t pid);



  tor::mojom::TorLauncherPtr tor_launcher_;

  TorLauncherCallback callback_;
  int64_t tor_pid_;

  base::FilePath::StringType path_;
  std::string host_;
  std::string port_;
  base::FilePath tor_data_path_;
  base::FilePath tor_watch_path_;
};

}  // namespace brave

#endif  // BRAVE_BROWSER_TOR_TOR_LAUNCHER_FACTORY_H_
