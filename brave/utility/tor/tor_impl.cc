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

#if defined(OS_POSIX)
int pipehack[2];

static void SIGCHLDHandler(int signal) {
  pid_t p;
  int status;

  if ((p = waitpid(-1, &status, WNOHANG | WUNTRACED)) != -1) {
    if (WIFSIGNALED(status) || WCOREDUMP(status)) {
      write(pipehack[1], &p, sizeof(pid_t));
    } else if (WIFEXITED(status)) {
      // TODO(darkdh): Handle "Address already in use" issue. Not notifying as
      // workaroud for now to prevent infinitely relaunch
      if (!WEXITSTATUS(status))
        write(pipehack[1], &p, sizeof(pid_t));
    }
  }
}

static void SetupPipeHack() {
  if (pipe(pipehack) == -1)
    LOG(ERROR) << "pipehack";

  int flags;
  for (size_t i = 0; i < 2; ++i) {
    if ((flags = fcntl(pipehack[i], F_GETFL)) == -1)
      LOG(ERROR) << "get flags";
    if (i == 1)
      flags |= O_NONBLOCK;
    flags |= O_CLOEXEC;
    if (fcntl(pipehack[i], F_SETFL, flags) == -1)
      LOG(ERROR) << "set flags";
  }

  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &action, NULL);

  memset(&action, 0, sizeof(action));
  action.sa_handler = SIGCHLDHandler;
  sigaction(SIGCHLD, &action, NULL);
}
#endif

namespace brave {

class TorLauncherDelegate : public base::LaunchOptions::PreExecDelegate {
 public:
    TorLauncherDelegate() {}
    ~TorLauncherDelegate() override {}

    void RunAsyncSafe() override {
#if defined(OS_POSIX)
      for (size_t i = 0; i < 2; ++i)
        close(pipehack[i]);
#endif
      // TODO(darkdh): doesn't prevent ctrl+c propagated by terminal
      struct sigaction action;
      memset(&action, 0, sizeof(action));
      action.sa_handler = SIG_IGN;
      sigaction(SIGINT, &action, NULL);
    }
 private:
    DISALLOW_COPY_AND_ASSIGN(TorLauncherDelegate);
};

TorLauncherImpl::TorLauncherImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {
  SetupPipeHack();
}

TorLauncherImpl::~TorLauncherImpl() {
  if (tor_process_.IsValid()) {
#if defined(OS_POSIX)
    for (size_t i = 0; i < 2; ++i)
      close(pipehack[i]);
#endif
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

void TorLauncherImpl::Launch(const base::FilePath& tor_bin,
                             const std::vector<std::string>& args,
                             LaunchCallback callback) {
  base::CommandLine cmdline(tor_bin);
  for (size_t i = 0; i < args.size(); ++i) {
    cmdline.AppendArg(args[i]);
  }
  base::LaunchOptions launchopts;
  TorLauncherDelegate tor_launcher_delegate;
  launchopts.pre_exec_delegate = &tor_launcher_delegate;
#if defined(OS_LINUX)
  launchopts.kill_on_parent_death = true;
#endif
  tor_process_ = base::LaunchProcess(cmdline, launchopts);

  bool result;
  // TODO(darkdh): return success when tor connected to tor network
  if (tor_process_.IsValid())
    result = true;
  else
    result = false;

  if (callback)
    std::move(callback).Run(result);

  if (!child_monitor_thread_.get()) {
    child_monitor_thread_.reset(new base::Thread("child_monitor_thread"));
    if (!child_monitor_thread_->Start()) {
      NOTREACHED();
    }

    child_monitor_thread_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&TorLauncherImpl::MonitorChild, base::Unretained(this)));
  }
}

void TorLauncherImpl::SetCrashHandler(SetCrashHandlerCallback callback) {
  crash_handler_callback_ = std::move(callback);
}

void TorLauncherImpl::MonitorChild() {
#if defined(OS_POSIX)
  pid_t pid;

  while (1) {
      if (read(pipehack[0], &pid, sizeof(pid_t)) > 0) {
        if (crash_handler_callback_)
          std::move(crash_handler_callback_).Run(pid);
        continue;
      } else {
        // pipes closed
        break;
      }
  }
#elif defined(OS_WINDOWS)
  WaitForSingleObject(tor_process_.Handle(), INFINITE);
  if (crash_handler_callback_)
    std::move(crash_handler_callback_).Run(pid);
#else
#error unsupported platforms
#endif
}

}  // namespace brave
