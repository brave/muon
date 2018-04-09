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
  int error = errno;
  char ch = 0;
  write(pipehack[1], &ch, 1);
  errno = error;
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
    if (fcntl(pipehack[i], F_SETFL, flags) == -1)
      LOG(ERROR) << "set flags";
    if ((flags = fcntl(pipehack[i], F_GETFD)) == -1)
      LOG(ERROR) << "get fd flags";
    flags |= FD_CLOEXEC;
    if (fcntl(pipehack[i], F_SETFD, flags) == -1)
      LOG(ERROR) << "set fd flags";
  }

  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = SIGCHLDHandler;
  sigaction(SIGCHLD, &action, NULL);
}
#endif

namespace brave {

#if defined(OS_POSIX)
class TorLauncherDelegate : public base::LaunchOptions::PreExecDelegate {
 public:
    TorLauncherDelegate() {}
    ~TorLauncherDelegate() override {}

    void RunAsyncSafe() override {
      for (size_t i = 0; i < 2; ++i)
        close(pipehack[i]);
      setsid();
    }
 private:
    DISALLOW_COPY_AND_ASSIGN(TorLauncherDelegate);
};
#endif

TorLauncherImpl::TorLauncherImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {
#if defined(OS_POSIX)
  SetupPipeHack();
#endif
}

TorLauncherImpl::~TorLauncherImpl() {
  if (tor_process_.IsValid()) {
    tor_process_.Terminate(0, false);
#if defined(OS_POSIX)
    for (size_t i = 0; i < 2; ++i)
      close(pipehack[i]);
#endif
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
                             const std::string& tor_host,
                             const std::string& tor_port,
                             const base::FilePath& tor_data_dir,
                             LaunchCallback callback) {
  base::CommandLine args(tor_bin);
  args.AppendArg("--ignore-missing-torrc");
  args.AppendArg("-f");
  args.AppendArg("/nonexistent");
  args.AppendArg("--defaults-torrc");
  args.AppendArg("/nonexistent");
  args.AppendArg("--SocksPort");
  args.AppendArg(tor_host + ":" + tor_port);
  if (!tor_data_dir.empty()) {
    args.AppendArg("--DataDirectory");
    args.AppendArgPath(tor_data_dir);
  }

  base::LaunchOptions launchopts;
#if defined(OS_POSIX)
  TorLauncherDelegate tor_launcher_delegate;
  launchopts.pre_exec_delegate = &tor_launcher_delegate;
#endif
#if defined(OS_LINUX)
  launchopts.kill_on_parent_death = true;
#endif
  tor_process_ = base::LaunchProcess(args, launchopts);

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
  char buf[PIPE_BUF];

  while (1) {
      if (read(pipehack[0], buf, sizeof(buf)) > 0) {
        pid_t pid;
        int status;

        if ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) != -1) {
          if (WIFSIGNALED(status)) {
            LOG(ERROR) << "tor got terminated by signal " << WTERMSIG(status);
          } else if (WCOREDUMP(status)) {
            LOG(ERROR) << "tor coredumped";
          } else if (WIFEXITED(status)) {
            LOG(ERROR) << "tor exit (" << WEXITSTATUS(status) << ")";
          }
          if (crash_handler_callback_)
            std::move(crash_handler_callback_).Run(pid);
        }
      } else {
        // pipes closed
        break;
      }
  }
#elif defined(OS_WIN)
  WaitForSingleObject(tor_process_.Handle(), INFINITE);
  if (crash_handler_callback_)
    std::move(crash_handler_callback_).
      Run(base::GetProcId(tor_process_.Handle()));
#else
#error unsupported platforms
#endif
}

}  // namespace brave
