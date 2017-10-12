// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/app/atom_main.h"

#include <stdlib.h>

#include <memory>
#include <string>

#include "atom/app/atom_main_delegate.h"
#include "atom/app/uv_task_runner.h"
#include "atom/common/atom_command_line.h"
#include "atom/common/options_switches.h"
#include "base/environment.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/app/content_main.h"
#include "content/public/common/content_switches.h"

#if defined(OS_POSIX) && !defined(OS_MACOSX)
#include "base/nix/xdg_util.h"
#endif

#if defined(OS_WIN)
#include <windows.h>  // windows.h must be included first

#include <shellapi.h>
#include <shellscalingapi.h>
#include <tchar.h>

#include "base/debug/dump_without_crashing.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/win_util.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/install_static/initialize_from_primary_module.h"
#include "chrome/install_static/install_details.h"
#include "chrome/install_static/install_util.h"
#include "chrome/install_static/product_install_details.h"
#include "chrome_elf/chrome_elf_main.h"
#include "chrome_elf/crash/crash_helper.h"
#include "components/crash/content/app/crash_switches.h"
#include "components/crash/content/app/crashpad.h"
#include "components/crash/content/app/fallback_crash_handling_win.h"
#include "components/crash/content/app/run_as_crashpad_handler_win.h"
#include "content/public/app/sandbox_helper_win.h"
#include "sandbox/win/src/sandbox_types.h"

namespace {

bool HasValidWindowsPrefetchArgument(const base::CommandLine& command_line) {
  const base::char16 kPrefetchArgumentPrefix[] = L"/prefetch:";

  for (const auto& arg : command_line.argv()) {
    if (arg.size() == arraysize(kPrefetchArgumentPrefix) &&
        base::StartsWith(arg, kPrefetchArgumentPrefix,
                         base::CompareCase::SENSITIVE)) {
      return arg[arraysize(kPrefetchArgumentPrefix) - 1] >= L'1' &&
             arg[arraysize(kPrefetchArgumentPrefix) - 1] <= L'8';
    }
  }
  return false;
}

int RunFallbackCrashHandler(const base::CommandLine& cmd_line) {
  // Retrieve the product & version details we need to report the crash
  // correctly.
  wchar_t exe_file[MAX_PATH] = {};
  CHECK(::GetModuleFileName(nullptr, exe_file, arraysize(exe_file)));

  base::string16 product_name;
  base::string16 version;
  base::string16 channel_name;
  base::string16 special_build;
  install_static::GetExecutableVersionDetails(exe_file, &product_name, &version,
                                              &special_build, &channel_name);

  return crash_reporter::RunAsFallbackCrashHandler(
      cmd_line, base::UTF16ToUTF8(product_name), base::UTF16ToUTF8(version),
      base::UTF16ToUTF8(channel_name));
}

}  // namespace
#endif  // defined(OS_WIN)

#if defined(OS_MACOSX)
extern "C" {
__attribute__((visibility("default")))
int ChromeMain(int argc, const char* argv[]);
}
#endif  // OS_MACOSX

#if defined(OS_WIN)
int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t* cmd, int) {
  int argc = 0;
  wchar_t** argv_setup = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
  base::CommandLine::Init(0, nullptr);

  install_static::InitializeFromPrimaryModule();
#else  // OS_WIN
#if defined(OS_MACOSX)
int ChromeMain(int argc, const char* argv[]) {
#else  // OS_MACOSX
int main(int argc, const char* argv[]) {
#endif
  char** argv_setup = uv_setup_args(argc, const_cast<char**>(argv));
  base::CommandLine::Init(argc, argv_setup);
#endif  // OS_WIN
  int64_t exe_entry_point_ticks = 0;
  // const base::TimeTicks exe_entry_point_ticks = base::TimeTicks::Now();

  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();

  std::unique_ptr<base::Environment> environment(base::Environment::Create());
  base::FilePath user_data_dir;
#if defined(OS_WIN) || defined(OS_MACOSX)
  PathService::Get(base::DIR_APP_DATA, &user_data_dir);
#else
  user_data_dir = base::nix::GetXDGDirectory(environment.get(),
                                  base::nix::kXdgConfigHomeEnvVar,
                                  base::nix::kDotConfigDir);
#endif
  if (command_line->HasSwitch(atom::options::kUserDataDirName)) {
    user_data_dir = user_data_dir.Append(
        command_line->GetSwitchValuePath(atom::options::kUserDataDirName));
  } else {
    user_data_dir = user_data_dir.Append(FILE_PATH_LITERAL("brave"));
  }
  environment->SetVar("CHROME_USER_DATA_DIR", user_data_dir.AsUTF8Unsafe());

#if defined(OS_WIN)
  SignalInitializeCrashReporting();

  const std::string process_type =
      command_line->GetSwitchValueASCII(switches::kProcessType);

  // Confirm that an explicit prefetch profile is used for all process types
  // except for the browser process. Any new process type will have to assign
  // itself a prefetch id. See kPrefetchArgument* constants in
  // content_switches.cc for details.
  DCHECK(process_type.empty() ||
         HasValidWindowsPrefetchArgument(*command_line));

  if (process_type == crash_reporter::switches::kCrashpadHandler) {
    crash_reporter::SetupFallbackCrashHandling(*command_line);
    return crash_reporter::RunAsCrashpadHandler(
        *base::CommandLine::ForCurrentProcess(), user_data_dir,
        switches::kProcessType, switches::kUserDataDir);
  } else if (process_type == crash_reporter::switches::kFallbackCrashHandler) {
    return RunFallbackCrashHandler(*command_line);
  }

  // Signal Chrome Elf that Chrome has begun to start.
  SignalChromeElf();

  // Initialize the sandbox services.
  sandbox::SandboxInterfaceInfo sandbox_info = {0};
  const bool is_browser = process_type.empty();
  const bool is_sandboxed = !command_line->HasSwitch(switches::kNoSandbox);
  if (is_browser || is_sandboxed) {
    // For child processes that are running as --no-sandbox, don't initialize
    // the sandbox info, otherwise they'll be treated as brokers (as if they
    // were the browser).
    content::InitializeSandboxInfo(&sandbox_info);
  }

#endif
  atom::AtomMainDelegate chrome_main_delegate(
      base::TimeTicks::FromInternalValue(exe_entry_point_ticks));
  content::ContentMainParams params(&chrome_main_delegate);

#if 0
  // The process should crash when going through abnormal termination.
  base::win::SetShouldCrashOnProcessDetach(true);
  base::win::SetAbortBehaviorForCrashReporting();
  params.instance = instance;
  params.sandbox_info = &sandbox_info;

  // SetDumpWithoutCrashingFunction must be passed the DumpProcess function
  // from chrome_elf and not from the DLL in order for DumpWithoutCrashing to
  // function correctly.
  typedef void (__cdecl *DumpProcessFunction)();
  DumpProcessFunction DumpProcess = reinterpret_cast<DumpProcessFunction>(
      ::GetProcAddress(::GetModuleHandle(chrome::kChromeElfDllName),
                       "DumpProcessWithoutCrash"));
  CHECK(DumpProcess);
  base::debug::SetDumpWithoutCrashingFunction(DumpProcess);

  atom::AtomCommandLine::InitW(argc, argv_setup);
#else
  params.argc = argc;
  params.argv = argv;
  atom::AtomCommandLine::Init(argc, argv_setup);
#endif  // defined(OS_WIN)

  int rv = content::ContentMain(params);

#if defined(OS_WIN)
  base::win::SetShouldCrashOnProcessDetach(false);
#endif

  return rv;
}
