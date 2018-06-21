// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_TOR_TOR_LAUNCHER_FACTORY_H_
#define BRAVE_BROWSER_TOR_TOR_LAUNCHER_FACTORY_H_

#include <string>

#include "brave/common/tor/tor.mojom.h"

namespace brave {

class TorLauncherFactory {
 public:
  TorLauncherFactory(const base::FilePath::StringType& path,
                      const std::string& proxy);
  ~TorLauncherFactory();

  void LaunchTorProcess();
  void RelaunchTorProcess();

 private:
  void LaunchOnLauncherThread();
  void RelaunchOnLauncherThread();

  void OnTorLauncherCrashed();
  void OnTorCrashed(int64_t pid);
  void OnTorLaunched(bool result);



  tor::mojom::TorLauncherPtr tor_launcher_;

  base::FilePath::StringType path_;
  std::string host_;
  std::string port_;
  base::FilePath tor_data_path_;
  base::FilePath tor_watch_path_;
};

}  // namespace brave

#endif  // BRAVE_BROWSER_TOR_TOR_LAUNCHER_FACTORY_H_
