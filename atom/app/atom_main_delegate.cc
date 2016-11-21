// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_main_delegate.h"

#include <iostream>
#include <string>
#include <vector>

#include "atom/app/atom_content_client.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/relauncher.h"
#include "atom/common/google_api_key.h"
#include "atom/renderer/atom_renderer_client.h"
#include "atom/utility/atom_content_utility_client.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/debug/stack_trace.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/trace_event/trace_log.h"
#include "brave/common/brave_paths.h"
#include "brightray/browser/brightray_paths.h"
#include "brightray/common/application_info.h"
#include "browser/brightray_paths.h"
#include "chrome/common/crash_keys.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/logging_chrome.h"
#include "chrome/common/trace_event_args_whitelist.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/crash/content/app/crashpad.h"
#include "content/public/common/content_switches.h"
#include "extensions/common/constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_switches.h"

#if defined(OS_WIN)
#include <atlbase.h>
#include <malloc.h>
#include <algorithm>
#include "base/debug/close_handle_hook_win.h"
#include "chrome/browser/downgrade/user_data_downgrade.h"
#include "chrome/common/child_process_logging.h"
#include "chrome/common/v8_breakpad_support_win.h"
#include "components/crash/content/app/crashpad.h"
#include "sandbox/win/src/sandbox.h"
#include "ui/base/resource/resource_bundle_win.h"
#endif

#if defined(OS_MACOSX)
#include "base/mac/foundation_util.h"
#include "chrome/app/chrome_main_mac.h"
#include "chrome/browser/mac/relauncher.h"
#include "chrome/common/mac/cfbundle_blocker.h"
#include "components/crash/content/app/crashpad.h"
#include "components/crash/core/common/objc_zombie.h"
#include "ui/base/l10n/l10n_util_mac.h"
#endif

#if defined(OS_POSIX)
#include <locale.h>
#include <signal.h>
#include "chrome/app/chrome_crash_reporter_client.h"
#endif

#if defined(ENABLE_EXTENSIONS)
#include "brave/browser/brave_content_browser_client.h"
#include "brave/renderer/brave_content_renderer_client.h"
#endif

namespace atom {

namespace {

const char* kRelauncherProcess = "relauncher";

bool IsBrowserProcess(base::CommandLine* cmd) {
  std::string process_type = cmd->GetSwitchValueASCII(::switches::kProcessType);
  return process_type.empty();
}

#if defined(OS_WIN)
// If we try to access a path that is not currently available, we want the call
// to fail rather than show an error dialog.
void SuppressWindowsErrorDialogs() {
  UINT new_flags = SEM_FAILCRITICALERRORS |
                   SEM_NOOPENFILEERRORBOX;

  // Preserve existing error mode.
  UINT existing_flags = SetErrorMode(new_flags);
  SetErrorMode(existing_flags | new_flags);
}

bool IsSandboxedProcess() {
  typedef bool (*IsSandboxedProcessFunc)();
  IsSandboxedProcessFunc is_sandboxed_process_func =
      reinterpret_cast<IsSandboxedProcessFunc>(
          GetProcAddress(GetModuleHandle(NULL), "IsSandboxedProcess"));
  return is_sandboxed_process_func && is_sandboxed_process_func();
}

void InvalidParameterHandler(const wchar_t*, const wchar_t*, const wchar_t*,
                             unsigned int, uintptr_t) {
  // noop.
}

bool UseHooks() {
#if defined(ARCH_CPU_X86_64)
  return false;
#elif defined(NDEBUG)
  return false;
#else  // NDEBUG
  return true;
#endif
}

#endif  // defined(OS_WIN)

struct MainFunction {
  const char* name;
  int (*function)(const content::MainFunctionParams&);
};

void InitLogging(const std::string& process_type) {
  logging::OldFileDeletionState file_state =
      logging::APPEND_TO_OLD_LOG_FILE;
  if (process_type.empty()) {
    file_state = logging::DELETE_OLD_LOG_FILE;
  }
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  logging::InitChromeLogging(command_line, file_state);
}

base::FilePath InitializeUserDataDir() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  base::FilePath user_data_dir;

  // first check the env
  std::string user_data_dir_string;
  std::unique_ptr<base::Environment> environment(base::Environment::Create());
  if (environment->GetVar("BRAVE_USER_DATA_DIR", &user_data_dir_string) &&
      base::IsStringUTF8(user_data_dir_string)) {
    user_data_dir = base::FilePath::FromUTF8Unsafe(user_data_dir_string);
    command_line->AppendSwitchPath(switches::kUserDataDir, user_data_dir);
  }

  // next check the user-data-dir switch
  if (user_data_dir.empty() ||
      command_line->HasSwitch(switches::kUserDataDir)) {
    user_data_dir =
      command_line->GetSwitchValuePath(switches::kUserDataDir);
    if (!user_data_dir.empty() && !user_data_dir.IsAbsolute()) {
      base::FilePath app_data_dir;
      PathService::Get(brightray::DIR_APP_DATA, &app_data_dir);
      user_data_dir = app_data_dir.Append(user_data_dir);
    }
  }

  // On Windows, trailing separators leave Chrome in a bad state.
  // See crbug.com/464616.
  if (user_data_dir.EndsWithSeparator())
    user_data_dir = user_data_dir.StripTrailingSeparators();

  if (!user_data_dir.empty())
    PathService::OverrideAndCreateIfNeeded(brightray::DIR_USER_DATA,
          user_data_dir, false, true);
    PathService::Override(chrome::DIR_USER_DATA, user_data_dir);
  // TODO(bridiver) Warn and fail early if the process fails
  // to get a user data directory.
  if (!PathService::Get(brightray::DIR_USER_DATA, &user_data_dir)) {
    brave::GetDefaultUserDataDirectory(&user_data_dir);
    PathService::OverrideAndCreateIfNeeded(brightray::DIR_USER_DATA,
            user_data_dir, false, true);
    PathService::Override(chrome::DIR_USER_DATA, user_data_dir);
  }

  return user_data_dir;
}

}  // namespace

AtomMainDelegate::AtomMainDelegate() {
}

AtomMainDelegate::~AtomMainDelegate() {
}

bool AtomMainDelegate::BasicStartupComplete(int* exit_code) {
  auto command_line = base::CommandLine::ForCurrentProcess();

#if defined(OS_MACOSX)
  // Give the browser process a longer treadmill, since crashes
  // there have more impact.
  const bool is_browser = !command_line->HasSwitch(switches::kProcessType);
  ObjcEvilDoers::ZombieEnable(true, is_browser ? 10000 : 1000);
#endif

  base::trace_event::TraceLog::GetInstance()->SetArgumentFilterPredicate(
      base::Bind(&IsTraceEventArgsWhitelisted));

#if defined(OS_WIN)
  v8_breakpad_support::SetUp();
#endif

#if defined(OS_WIN)
  if (UseHooks())
    base::debug::InstallHandleHooks();
  else
    base::win::DisableHandleVerifier();

  // Ignore invalid parameter errors.
  _set_invalid_parameter_handler(InvalidParameterHandler);
#endif

  chrome::RegisterPathProvider();

  ContentSettingsPattern::SetNonWildcardDomainNonPortScheme(
      extensions::kExtensionScheme);

#if defined(OS_MACOSX)
  SetUpBundleOverrides();
#endif

  return brightray::MainDelegate::BasicStartupComplete(exit_code);
}

void AtomMainDelegate::PreSandboxStartup() {
  brightray::MainDelegate::PreSandboxStartup();

  auto command_line = base::CommandLine::ForCurrentProcess();
  std::string process_type = command_line->GetSwitchValueASCII(
      ::switches::kProcessType);

#if defined(OS_POSIX)
  // crash_reporter::SetCrashReporterClient(g_chrome_crash_client.Pointer());
#endif

#if defined(OS_MACOSX)
#ifdef DEBUG
  // disable os crash dumps in debug mode because they take forever
  base::mac::DisableOSCrashDumps();
#endif
  InitMacCrashReporter(command_line, process_type);
#endif

#if defined(OS_WIN)
  child_process_logging::Init();
#endif

  base::FilePath path = InitializeUserDataDir();
  if (path.empty()) {
    LOG(ERROR) << "Could not create user data dir";
  } else {
    PathService::OverrideAndCreateIfNeeded(
        component_updater::DIR_COMPONENT_USER,
        path.Append(FILE_PATH_LITERAL("Extensions")), false, true);
  }

#if !defined(OS_WIN)
  // For windows we call InitLogging when the sandbox is initialized.
  InitLogging(process_type);
#endif

  // Set google API key.
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (!env->HasVar("GOOGLE_API_ENDPOINT"))
    env->SetVar("GOOGLE_API_ENDPOINT", GOOGLEAPIS_ENDPOINT);
  if (!env->HasVar("GOOGLE_API_KEY"))
    env->SetVar("GOOGLE_API_KEY", GOOGLEAPIS_API_KEY);

#if !defined(CHROME_MULTIPLE_DLL_BROWSER)
  if (process_type == switches::kUtilityProcess ||
      process_type == switches::kZygoteProcess) {
    AtomContentUtilityClient::PreSandboxStartup();
  }
#endif

#if defined(OS_POSIX) && !defined(OS_MACOSX)
  // Zygote needs to call InitCrashReporter() in RunZygote().
  if (process_type != switches::kZygoteProcess) {
//    breakpad::InitCrashReporter(process_type);
  }
#endif  // defined(OS_POSIX) && !defined(OS_MACOSX)

  // Only append arguments for browser process.
  if (!IsBrowserProcess(command_line))
    return;

#if defined(OS_LINUX)
  // always disable the sandbox on linux for now
  // https://github.com/brave/browser-laptop/issues/715
  command_line->AppendSwitch(::switches::kNoSandbox);
#endif
}

#if defined(OS_POSIX) && !defined(OS_MACOSX)
void AtomMainDelegate::ZygoteForked() {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(
          switches::kEnableCrashReporter)) {
    std::string process_type =
        command_line->GetSwitchValueASCII(
            switches::kProcessType);
    //breakpad::InitCrashReporter(process_type);
    // Reset the command line for the newly spawned process.
    crash_keys::SetCrashKeysFromCommandLine(*command_line);
  }
}
#endif

content::ContentBrowserClient* AtomMainDelegate::CreateContentBrowserClient() {
  browser_client_.reset(new brave::BraveContentBrowserClient);
  return browser_client_.get();
}

content::ContentRendererClient*
    AtomMainDelegate::CreateContentRendererClient() {
  renderer_client_.reset(new brave::BraveContentRendererClient);
  return renderer_client_.get();
}

content::ContentUtilityClient* AtomMainDelegate::CreateContentUtilityClient() {
  utility_client_.reset(new AtomContentUtilityClient);
  return utility_client_.get();
}

int AtomMainDelegate::RunProcess(
    const std::string& process_type,
    const content::MainFunctionParams& main_function_params) {
    static const MainFunction kMainFunctions[] = {
  #if defined(ENABLE_PRINT_PREVIEW) && !defined(CHROME_MULTIPLE_DLL_CHILD)
      { switches::kServiceProcess,     ServiceProcessMain },
  #endif

  #if defined(OS_MACOSX)
      { switches::kRelauncherProcess,
        relauncher::RelauncherMain },
        // mac_relauncher::internal::RelauncherMain },
  #endif
      { "<invalid>", NULL },  // To avoid constant array of size 0
    };

    for (size_t i = 0; i < arraysize(kMainFunctions); ++i) {
      if (process_type == kMainFunctions[i].name)
        return kMainFunctions[i].function(main_function_params);
    }

    return -1;
}

void AtomMainDelegate::SandboxInitialized(const std::string& process_type) {
#if defined(OS_WIN)
  InitLogging(process_type);
  SuppressWindowsErrorDialogs();
#endif
}

#if defined(OS_MACOSX)
bool AtomMainDelegate::ShouldSendMachPort(const std::string& process_type) {
  return process_type != switches::kRelauncherProcess &&
      process_type != switches::kServiceProcess;
}

bool AtomMainDelegate::DelaySandboxInitialization(
    const std::string& process_type) {
  return process_type == kRelauncherProcess;
}

void AtomMainDelegate::InitMacCrashReporter(
    base::CommandLine* command_line,
    const std::string& process_type) {
  // TODO(mark): Right now, InitializeCrashpad() needs to be called after
  // CommandLine::Init() and chrome::RegisterPathProvider().  Ideally, Crashpad
  // initialization could occur sooner, preferably even before the framework
  // dylib is even loaded, to catch potential early crashes.

  const bool browser_process = process_type.empty();
  const bool install_from_dmg_relauncher_process =
      process_type == switches::kRelauncherProcess &&
      command_line->HasSwitch(switches::kRelauncherProcessDMGDevice);

  const bool initial_client =
      browser_process || install_from_dmg_relauncher_process;

  // TODO(bridiver) - unresolved symol issue
  // crash_reporter::InitializeCrashpad(initial_client, process_type);

  if (!browser_process) {
    std::string metrics_client_id =
        command_line->GetSwitchValueASCII(switches::kMetricsClientID);
    crash_keys::SetMetricsClientIdFromGUID(metrics_client_id);
  }

  // Mac is packaged with a main app bundle and a helper app bundle.
  // The main app bundle should only be used for the browser process, so it
  // should never see a --type switch (switches::kProcessType).  Likewise,
  // the helper should always have a --type switch.
  //
  // This check is done this late so there is already a call to
  // base::mac::IsBackgroundOnlyProcess(), so there is no change in
  // startup/initialization order.

  // The helper's Info.plist marks it as a background only app.
  if (base::mac::IsBackgroundOnlyProcess()) {
    CHECK(command_line->HasSwitch(switches::kProcessType) &&
          !process_type.empty())
        << "Helper application requires --type.";
  } else {
    CHECK(!command_line->HasSwitch(switches::kProcessType) &&
          process_type.empty())
        << "Main application forbids --type, saw " << process_type;
  }
}
#endif  // OS_MACOSX

void AtomMainDelegate::ProcessExiting(const std::string& process_type) {
  brightray::MainDelegate::ProcessExiting(process_type);
  logging::CleanupChromeLogging();
#if defined(OS_WIN)
  base::debug::RemoveHandleHooks();
#endif
}

std::unique_ptr<brightray::ContentClient>
AtomMainDelegate::CreateContentClient() {
  return std::unique_ptr<brightray::ContentClient>(new AtomContentClient);
}

}  // namespace atom
