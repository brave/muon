// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <utility>

#include "atom/browser/atom_browser_main_parts.h"

#include "atom/browser/api/trackable_object.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/bridge_task_runner.h"
#include "atom/browser/browser.h"
#include "atom/browser/browser_context_keyed_service_factories.h"
#include "atom/browser/javascript_environment.h"
#include "atom/common/api/atom_bindings.h"
#include "atom/common/node_bindings.h"
#include "atom/common/node_includes.h"
#include "base/allocator/allocator_extension.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/file_util.h"
#include "base/memory/memory_pressure_monitor.h"
#include "base/path_service.h"
#include "base/profiler/stack_sampling_profiler.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/default_tick_clock.h"
#include "base/trace_event/trace_event.h"
#include "browser/media/media_capture_devices_dispatcher.h"
#include "chrome/browser/browser_shutdown.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/webui/chrome_web_ui_controller_factory.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_result_codes.h"
#include "chrome/common/chrome_switches.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/web_ui_controller_factory.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "device/geolocation/geolocation_provider.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "muon/app/muon_crash_reporter_client.h"
#include "muon/browser/muon_browser_process_impl.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "v8/include/v8.h"
#include "v8/include/v8-debug.h"

#if defined(OS_MACOSX)
#include <Security/Security.h>
#endif  // defined(OS_MACOSX)

#if defined(OS_LINUX)
#include "brave/grit/brave_strings.h"  // NOLINT: This file is generated
#include "chrome/common/chrome_paths_internal.h"
#include "components/os_crypt/key_storage_config_linux.h"
#include "components/os_crypt/os_crypt.h"
#include "ui/base/l10n/l10n_util.h"
#endif

#if defined(USE_X11)
#include "chrome/browser/ui/libgtkui/gtk_util.h"
#include "ui/events/devices/x11/touch_factory_x11.h"
#include "ui/views/linux_ui/linux_ui.h"
#endif

#if defined(USE_AURA)
#include "chrome/browser/lifetime/application_lifetime.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/runner/common/client_util.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#endif

#if defined(OS_WIN)
#include "base/win/pe_image.h"
#include "base/win/registry.h"
#include "base/win/win_util.h"
#include "base/win/windows_version.h"
#include "base/win/wrapped_window_proc.h"
#include "components/crash/content/app/crash_export_thunks.h"
#endif

namespace atom {

namespace {

#if defined(OS_MACOSX)
OSStatus KeychainCallback(SecKeychainEvent keychain_event,
                          SecKeychainCallbackInfo* info, void* context) {
  return noErr;
}
#endif  // defined(OS_MACOSX)

#if defined(OS_WIN)
void InitializeWindowProcExceptions() {
  base::win::WinProcExceptionFilter exception_filter =
      base::win::SetWinProcExceptionFilter(&CrashForException_ExportThunk);
  DCHECK(!exception_filter);
}
#endif  // defined (OS_WIN)

}  // namespace

template<typename T>
void Erase(T* container, typename T::iterator iter) {
  container->erase(iter);
}

// static
AtomBrowserMainParts* AtomBrowserMainParts::self_ = nullptr;

AtomBrowserMainParts::AtomBrowserMainParts(
    const content::MainFunctionParams& parameters)
    : exit_code_(nullptr),
      parameters_(parameters),
      parsed_command_line_(parameters.command_line),
      browser_(new Browser),
      node_bindings_(NodeBindings::Create()),
      atom_bindings_(new AtomBindings),
      gc_timer_(true, true) {
  DCHECK(!self_) << "Cannot have two AtomBrowserMainParts";
  self_ = this;
}

AtomBrowserMainParts::~AtomBrowserMainParts() {
  // Leak the JavascriptEnvironment on exit.
  // This is to work around the bug that V8 would be waiting for background
  // tasks to finish on exit, while somehow it waits forever in Electron, more
  // about this can be found at https://github.com/electron/electron/issues/4767.
  // On the other handle there is actually no need to gracefully shutdown V8
  // on exit in the main process, we already ensured all necessary resources get
  // cleaned up, and it would make quitting faster.
  ignore_result(js_env_.release());
}

// static
AtomBrowserMainParts* AtomBrowserMainParts::Get() {
  DCHECK(self_);
  return self_;
}

bool AtomBrowserMainParts::SetExitCode(int code) {
  if (!exit_code_)
    return false;

  *exit_code_ = code;
  return true;
}

int AtomBrowserMainParts::GetExitCode() {
  return exit_code_ != nullptr ? *exit_code_ : 0;
}

base::Closure AtomBrowserMainParts::RegisterDestructionCallback(
    const base::Closure& callback) {
  auto iter = destructors_.insert(destructors_.end(), callback);
  return base::Bind(&Erase<std::list<base::Closure>>, &destructors_, iter);
}

int AtomBrowserMainParts::PreEarlyInitialization() {
  brightray::BrowserMainParts::PreEarlyInitialization();
#if defined(OS_POSIX)
  HandleSIGCHLD();
#endif
  return content::RESULT_CODE_NORMAL_EXIT;
}

int AtomBrowserMainParts::PreCreateThreads() {
  TRACE_EVENT0("startup", "AtomBrowserMainParts::PreCreateThreads")

  base::FilePath user_data_dir;
  if (!PathService::Get(chrome::DIR_USER_DATA, &user_data_dir))
    return chrome::RESULT_CODE_MISSING_DATA;

  // Force MediaCaptureDevicesDispatcher to be created on UI thread.
  brightray::MediaCaptureDevicesDispatcher::GetInstance();
  scoped_refptr<base::SequencedTaskRunner> local_state_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

  {
    TRACE_EVENT0("startup",
      "AtomBrowserMainParts::PreCreateThreads:InitBrowswerProcessImpl");
    fake_browser_process_.reset(
       new MuonBrowserProcessImpl(local_state_task_runner.get()));
  }

#if defined(OS_MACOSX)
  // Get the Keychain API to register for distributed notifications on the main
  // thread, which has a proper CFRunloop, instead of later on the I/O thread,
  // which doesn't. This ensures those notifications will get delivered
  // properly. See issue 37766.
  // (Note that the callback mask here is empty. I don't want to register for
  // any callbacks, I just want to initialize the mechanism.)
  SecKeychainAddCallback(&KeychainCallback, 0, NULL);
#endif  // defined(OS_MACOSX)

  fake_browser_process_->PreCreateThreads(
      *base::CommandLine::ForCurrentProcess());

  MuonCrashReporterClient::InitCrashReporting();

#if defined(USE_AURA)
  // The screen may have already been set in test initialization.
  if (!display::Screen::GetScreen())
    display::Screen::SetScreenInstance(views::CreateDesktopScreen());
#if defined(USE_X11)
  views::LinuxUI::instance()->UpdateDeviceScaleFactor();
#endif
#endif

  return content::RESULT_CODE_NORMAL_EXIT;;
}

void AtomBrowserMainParts::PostEarlyInitialization() {
  brightray::BrowserMainParts::PostEarlyInitialization();
}

void AtomBrowserMainParts::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  if (atom::Browser::Get()->is_shutting_down())
    return;

  base::allocator::ReleaseFreeMemory();

  if (js_env_.get() && js_env_->isolate()) {
    js_env_->isolate()->LowMemoryNotification();
  }
}

void AtomBrowserMainParts::IdleHandler() {
  base::allocator::ReleaseFreeMemory();
}

#if defined(OS_WIN)
void AtomBrowserMainParts::PreMainMessageLoopStart() {
  brightray::BrowserMainParts::PreMainMessageLoopStart();
  if (!parameters_.ui_task) {
    // Make sure that we know how to handle exceptions from the message loop.
    InitializeWindowProcExceptions();
  }
}
#endif

void AtomBrowserMainParts::PreMainMessageLoopRun() {
#if defined(USE_AURA)
  if (content::ServiceManagerConnection::GetForProcess() &&
      service_manager::ServiceManagerIsRemote()) {
    content::ServiceManagerConnection::GetForProcess()->
        SetConnectionLostClosure(base::Bind(&chrome::SessionEnding));
  }
#endif

  // TODO(bridiver) - find a better place for these
  extensions::ExtensionApiFrameIdMap::Get();

  fake_browser_process_->PreMainMessageLoopRun();

  content::WebUIControllerFactory::RegisterFactory(
      ChromeWebUIControllerFactory::GetInstance());

  js_env_.reset(new JavascriptEnvironment);
  js_env_->isolate()->Enter();

  node_bindings_->Initialize();

  // Create the global environment.
  node::Environment* env =
      node_bindings_->CreateEnvironment(js_env_->context());

  // Add atom-shell extended APIs.
  atom_bindings_->BindTo(js_env_->isolate(), env->process_object());

  // Load everything.
  node_bindings_->LoadEnvironment(env);

  // Wrap the uv loop with global env.
  node_bindings_->set_uv_env(env);
#if defined(USE_X11)
  ui::TouchFactory::SetTouchDeviceListFromCommandLine();
#endif

  // Start idle gc.
  gc_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMinutes(1),
      base::Bind(&AtomBrowserMainParts::IdleHandler,
                 base::Unretained(this)));

  memory_pressure_listener_.reset(new base::MemoryPressureListener(
      base::Bind(&AtomBrowserMainParts::OnMemoryPressure,
        base::Unretained(this))));

  // Make sure the userData directory is created.
  base::FilePath user_data;
  if (PathService::Get(chrome::DIR_USER_DATA, &user_data))
    base::CreateDirectoryAndGetError(user_data, nullptr);

  // PreProfileInit
  EnsureBrowserContextKeyedServiceFactoriesBuilt();
  auto command_line = base::CommandLine::ForCurrentProcess();
#if defined(OS_LINUX)
  std::unique_ptr<os_crypt::Config> config(new os_crypt::Config());
  // Forward to os_crypt the flag to use a specific password store.
  config->store =
      command_line->GetSwitchValueASCII(switches::kPasswordStore);
  // Forward the product name
  config->product_name = l10n_util::GetStringUTF8(IDS_PRODUCT_NAME);
  // OSCrypt may target keyring, which requires calls from the main thread.
  config->main_thread_runner = content::BrowserThread::GetTaskRunnerForThread(
      content::BrowserThread::UI);
  // OSCrypt can be disabled in a special settings file.
  config->should_use_preference =
      command_line->HasSwitch(switches::kEnableEncryptionSelection);
  chrome::GetDefaultUserDataDirectory(&config->user_data_path);
  OSCrypt::SetConfig(std::move(config));
#endif

  browser_context_ = ProfileManager::GetActiveUserProfile();
  brightray::BrowserMainParts::PreMainMessageLoopRun();

  js_env_->OnMessageLoopCreated();
  node_bindings_->PrepareMessageLoop();
  node_bindings_->RunMessageLoop();

#if defined(USE_X11)
  libgtkui::GtkInitFromCommandLine(*base::CommandLine::ForCurrentProcess());
#endif

#if !defined(OS_MACOSX)
  // The corresponding call in macOS is in AtomApplicationDelegate.
  Browser::Get()->WillFinishLaunching();
  std::unique_ptr<base::DictionaryValue> empty_info(new base::DictionaryValue);
  Browser::Get()->DidFinishLaunching(*empty_info);
#endif

  base::FeatureList::InitializeInstance(
      command_line->GetSwitchValueASCII(switches::kEnableFeatures),
      command_line->GetSwitchValueASCII(switches::kDisableFeatures));
  auto feature_list = base::FeatureList::GetInstance();
  // temporary workaround for flash
  auto field_trial =
      feature_list->GetFieldTrial(features::kPreferHtmlOverPlugins);
  feature_list->RegisterFieldTrialOverride(
                    features::kPreferHtmlOverPlugins.name,
                    base::FeatureList::OVERRIDE_DISABLE_FEATURE, field_trial);
  // disable ukm metrics
  field_trial = feature_list->GetFieldTrial(ukm::kUkmFeature);
  feature_list->RegisterFieldTrialOverride(ukm::kUkmFeature.name,
                      base::FeatureList::OVERRIDE_DISABLE_FEATURE, field_trial);

  field_trial = feature_list->GetFieldTrial(
      features::kGuestViewCrossProcessFrames);
  feature_list->RegisterFieldTrialOverride(
                      features::kGuestViewCrossProcessFrames.name,
                      base::FeatureList::OVERRIDE_DISABLE_FEATURE, field_trial);

  // enable fill-on-account-select
  field_trial = feature_list->GetFieldTrial(
      password_manager::features::kFillOnAccountSelect);
  feature_list->RegisterFieldTrialOverride(
      password_manager::features::kFillOnAccountSelect.name,
      base::FeatureList::OVERRIDE_ENABLE_FEATURE, field_trial);
}



bool AtomBrowserMainParts::MainMessageLoopRun(int* result_code) {
  exit_code_ = result_code;
  return brightray::BrowserMainParts::MainMessageLoopRun(result_code);
}

void AtomBrowserMainParts::PostMainMessageLoopStart() {
  brightray::BrowserMainParts::PostMainMessageLoopStart();
#if defined(OS_POSIX)
  HandleShutdownSignals();
#endif
}

void AtomBrowserMainParts::PostMainMessageLoopRun() {
  browser_context_ = nullptr;
  brightray::BrowserMainParts::PostMainMessageLoopRun();

  js_env_->OnMessageLoopDestroying();

  js_env_->isolate()->Exit();
#if defined(OS_MACOSX)
  FreeAppDelegate();
#endif

  // Make sure destruction callbacks are called before message loop is
  // destroyed, otherwise some objects that need to be deleted on IO thread
  // won't be freed.
  // We don't use ranged for loop because iterators are getting invalided when
  // the callback runs.
  for (auto iter = destructors_.begin(); iter != destructors_.end();) {
    base::Closure& callback = *iter;
    ++iter;
    callback.Run();
  }

  restart_last_session_ = browser_shutdown::ShutdownPreThreadsStop();

  fake_browser_process_->StartTearDown();
}

void AtomBrowserMainParts::PostDestroyThreads() {
  int restart_flags = restart_last_session_
                          ? browser_shutdown::RESTART_LAST_SESSION
                          : browser_shutdown::NO_FLAGS;

  fake_browser_process_->PostDestroyThreads();
  // browser_shutdown takes care of deleting browser_process, so we need to
  // release it.
  ignore_result(fake_browser_process_.release());

  browser_shutdown::ShutdownPostThreadsStop(restart_flags);
}

}  // namespace atom
