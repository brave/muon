// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_main_delegate.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "atom/app/atom_content_client.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/relauncher.h"
#include "atom/common/atom_command_line.h"
#include "atom/common/google_api_key.h"
#include "atom/utility/atom_content_utility_client.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/debug/stack_trace.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/trace_event/trace_log.h"
#include "brightray/common/application_info.h"
#include "chrome/child/child_profiling.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_paths_internal.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/logging_chrome.h"
#include "chrome/common/profiling.h"
#include "chrome/common/trace_event_args_whitelist.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "content/public/common/content_switches.h"
#include "extensions/common/constants.h"
#include "extensions/features/features.h"
#include "muon/app/muon_crash_reporter_client.h"
#include "printing/features/features.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_switches.h"

#if defined(OS_WIN)
#include <atlbase.h>
#include <malloc.h>
#include <algorithm>
#include "base/debug/close_handle_hook_win.h"
#include "chrome/child/v8_breakpad_support_win.h"
#include "chrome/common/child_process_logging.h"
#include "chrome/install_static/install_details.h"
#include "chrome/install_static/install_util.h"
#include "chrome/install_static/product_install_details.h"
#include "sandbox/win/src/sandbox.h"
#include "ui/base/resource/resource_bundle_win.h"
#endif

#if defined(OS_MACOSX)
#include "chrome/app/chrome_main_mac.h"
#include "chrome/browser/mac/relauncher.h"
#include "chrome/common/mac/cfbundle_blocker.h"
#include "components/crash/core/common/objc_zombie.h"
#include "ui/base/l10n/l10n_util_mac.h"
#endif

#if defined(OS_POSIX)
#include <locale.h>
#include <signal.h>
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
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
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  base::FilePath user_data_dir;
#if defined(OS_WIN)
  wchar_t user_data_dir_buf[MAX_PATH], invalid_user_data_dir_buf[MAX_PATH];

  using GetUserDataDirectoryThunkFunction =
      void (*)(wchar_t*, size_t, wchar_t*, size_t);
  HMODULE elf_module = GetModuleHandle(chrome::kChromeElfDllName);
  if (elf_module) {
    // If we're in a test, chrome_elf won't be loaded.
    GetUserDataDirectoryThunkFunction get_user_data_directory_thunk =
        reinterpret_cast<GetUserDataDirectoryThunkFunction>(
            GetProcAddress(elf_module, "GetUserDataDirectoryThunk"));
    get_user_data_directory_thunk(
        user_data_dir_buf, arraysize(user_data_dir_buf),
        invalid_user_data_dir_buf, arraysize(invalid_user_data_dir_buf));
    if (invalid_user_data_dir_buf[0] == 0)
      user_data_dir = base::FilePath(user_data_dir_buf);
  } else {
    // In tests, just respect the flag if given.
    user_data_dir =
        command_line.GetSwitchValuePath(switches::kUserDataDir);
    if (!user_data_dir.empty() && user_data_dir.EndsWithSeparator())
      user_data_dir = user_data_dir.StripTrailingSeparators();
  }
#else  // OS_WIN
  user_data_dir =
      command_line.GetSwitchValuePath(switches::kUserDataDir);

  if (user_data_dir.empty()) {
    std::string user_data_dir_string;
    std::unique_ptr<base::Environment> environment(base::Environment::Create());
    if (environment->GetVar("CHROME_USER_DATA_DIR", &user_data_dir_string) &&
        base::IsStringUTF8(user_data_dir_string)) {
      user_data_dir = base::FilePath::FromUTF8Unsafe(user_data_dir_string);
    }
  }
#endif  // OS_WIN

  // Warn and fail early if the process fails to get a user data directory.
  if (user_data_dir.empty() ||
      !PathService::OverrideAndCreateIfNeeded(chrome::DIR_USER_DATA,
          user_data_dir, false, true)) {
    std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);
    // The browser process (which is identified by an empty |process_type|) will
    // handle the error later; other processes that need the dir crash here.
    CHECK(process_type.empty()) << "Unable to get the user data directory "
                                << "for process type: " << process_type;
  }

#if defined(OS_POSIX)
  // Setup NativeMessagingHosts to point to the default Chrome locations
  // because that's where native apps will create them
  base::FilePath chrome_user_data_dir;
  base::FilePath native_messaging_dir;
#if defined(OS_MACOSX)
  PathService::Get(base::DIR_APP_DATA, &chrome_user_data_dir);
  chrome_user_data_dir = chrome_user_data_dir.Append("Google/Chrome");
  native_messaging_dir = base::FilePath(FILE_PATH_LITERAL(
      "/Library/Google/Chrome/NativeMessagingHosts"));
#else
  chrome::GetDefaultUserDataDirectory(&chrome_user_data_dir);
  native_messaging_dir = base::FilePath(FILE_PATH_LITERAL(
      "/etc/opt/chrome/native-messaging-hosts"));
#endif  // defined(OS_MACOSX)
  PathService::OverrideAndCreateIfNeeded(
      chrome::DIR_USER_NATIVE_MESSAGING,
      chrome_user_data_dir.Append(FILE_PATH_LITERAL("NativeMessagingHosts")),
      false, true);
  PathService::OverrideAndCreateIfNeeded(chrome::DIR_NATIVE_MESSAGING,
      native_messaging_dir, false, true);
#endif  // OS_POSIX
  return user_data_dir;
}

#if defined(OS_POSIX) && !defined(OS_MACOSX)
void SIGTERMProfilingShutdown(int signal) {
  Profiling::Stop();
  struct sigaction sigact;
  memset(&sigact, 0, sizeof(sigact));
  sigact.sa_handler = SIG_DFL;
  CHECK_EQ(sigaction(SIGTERM, &sigact, NULL), 0);
  raise(signal);
}

void SetUpProfilingShutdownHandler() {
  struct sigaction sigact;
  sigact.sa_handler = SIGTERMProfilingShutdown;
  sigact.sa_flags = SA_RESETHAND;
  sigemptyset(&sigact.sa_mask);
  CHECK_EQ(sigaction(SIGTERM, &sigact, NULL), 0);
}
#endif  // !defined(OS_MACOSX) && !defined(OS_ANDROID)

}  // namespace

AtomMainDelegate::AtomMainDelegate()
    : AtomMainDelegate(base::TimeTicks()) {}

AtomMainDelegate::AtomMainDelegate(base::TimeTicks exe_entry_point_ticks) {
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

  SetUpBundleOverrides();
  chrome::common::mac::EnableCFBundleBlocker();
#endif

#if !defined(CHROME_MULTIPLE_DLL_BROWSER)
  ChildProfiling::ProcessStarted();
#endif
  Profiling::ProcessStarted();

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

  return brightray::MainDelegate::BasicStartupComplete(exit_code);
}

void AtomMainDelegate::PreSandboxStartup() {
#if defined(OS_WIN)
  install_static::InitializeProcessType();
#endif

  base::FilePath path = InitializeUserDataDir();
  if (path.empty()) {
    LOG(ERROR) << "Could not create user data dir";
  } else {
    PathService::OverrideAndCreateIfNeeded(
        component_updater::DIR_COMPONENT_USER,
        path.Append(FILE_PATH_LITERAL("Extensions")), false, true);
  }

  MuonCrashReporterClient::InitForProcess();

  auto command_line = base::CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line->GetSwitchValueASCII(::switches::kProcessType);

#if !defined(OS_WIN)
  // For windows we call InitLogging when the sandbox is initialized.
  InitLogging(process_type);
#else
  child_process_logging::Init();
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

#if defined(OS_LINUX)
  if (!IsBrowserProcess(command_line)) {
    // Disable setuid sandbox
    command_line->AppendSwitch(::switches::kDisableSetuidSandbox);
  }
#endif

  brightray::MainDelegate::PreSandboxStartup();
}

#if defined(OS_POSIX) && !defined(OS_MACOSX)
void AtomMainDelegate::ZygoteForked() {
  ChildProfiling::ProcessStarted();
  Profiling::ProcessStarted();
  if (Profiling::BeingProfiled()) {
    base::debug::RestartProfilingAfterFork();
    SetUpProfilingShutdownHandler();
  }

  MuonCrashReporterClient::InitForProcess();
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
  #if BUILDFLAG(ENABLE_PRINT_PREVIEW) && !defined(CHROME_MULTIPLE_DLL_CHILD)
      { switches::kCloudPrintServiceProcess, CloudPrintServiceProcessMain },
  #endif

  #if defined(OS_MACOSX)
      { switches::kRelauncherProcess, relauncher::RelauncherMain },
  #else
      { kRelauncherProcess, relauncher::RelauncherMain },
  #endif
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
      process_type != switches::kCloudPrintServiceProcess;
}

bool AtomMainDelegate::DelaySandboxInitialization(
    const std::string& process_type) {
  return process_type == kRelauncherProcess;
}
#endif  // OS_MACOSX

void AtomMainDelegate::ProcessExiting(const std::string& process_type) {
  brightray::MainDelegate::ProcessExiting(process_type);
  logging::CleanupChromeLogging();
}

std::unique_ptr<brightray::ContentClient>
AtomMainDelegate::CreateContentClient() {
  return std::unique_ptr<brightray::ContentClient>(new AtomContentClient);
}

}  // namespace atom
