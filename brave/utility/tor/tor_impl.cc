// Copyright (c) 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/utility/tor/tor_impl.h"

#include <utility>

#include "base/command_line.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"

namespace brave {

TorLauncherImpl::TorLauncherImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {}

TorLauncherImpl::~TorLauncherImpl() {
  if (tor_process_.IsValid()) {
    tor_process_.Terminate(0, false);
#if defined(OS_MACOSX)
    base::PostTaskWithTraits(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
        base::BindOnce(&base::EnsureProcessTerminated, Passed(&tor_process_)));
#else
    base::EnsureProcessTerminated(std::move(tor_process_));
#endif
  }
}

#if defined(OS_POSIX)
void SIGCHLDHandler(int signal) {
  pid_t p;
  int status;

  while ((p = waitpid(-1, &status, WNOHANG)) != -1) {
    LOG(ERROR) << "Tor process terminated";
    // TODO(darkdh): handle tor got killed by other factors
  }
}
#endif

void TorLauncherImpl::Launch(const base::FilePath& tor_bin,
                             const std::vector<std::string>& args,
                             LaunchCallback callback) {
  if (tor_process_.IsValid())
    return;
  base::CommandLine cmdline(tor_bin);
  for (size_t i = 0; i < args.size(); ++i) {
    cmdline.AppendArg(args[i]);
  }
  base::LaunchOptions launchopts;
#if defined(OS_LINUX)
  launchopts.kill_on_parent_death = true;
#endif
  tor_process_ = base::LaunchProcess(cmdline, launchopts);

  bool result;
  if (tor_process_.IsValid())
    result = true;
  else
    result = false;

  if (callback)
    std::move(callback).Run(result);

  child_monitor_thread_.reset(new base::Thread("child_monitor_thread"));
  if (!child_monitor_thread_->Start()) {
    NOTREACHED();
  }

  child_monitor_thread_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&TorLauncherImpl::MonitorChild, base::Unretained(this)));
}

void TorLauncherImpl::IsTorAlive(IsTorAliveCallback callback) {
  if (callback)
    std::move(callback).Run(tor_process_.IsValid());
}

void TorLauncherImpl::MonitorChild() {
#if defined(OS_POSIX)
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = SIGCHLDHandler;
  sigaction(SIGCHLD, &action, NULL);
#elif defined(OS_WINDOWS)
  WaitForSingleObject(tor_process_.Handle(), INFINITE);
  LOG(ERROR) << "Tor process terminated";
  // TODO(darkdh): handle tor got killed by other factors
#else
#error unsupported platforms
#endif
}

}  // namespace brave
