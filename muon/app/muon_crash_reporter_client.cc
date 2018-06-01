// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "muon/app/muon_crash_reporter_client.h"

#include "atom/common/atom_version.h"
#include "atom/common/options_switches.h"
#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/debug/leak_annotations.h"
#include "base/path_service.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/crash_keys.h"
#include "chrome/installer/util/google_update_settings.h"
#include "components/crash/content/app/crashpad.h"
#include "components/crash/core/common/crash_key.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/common/content_switches.h"
#include "services/service_manager/embedder/switches.h"

#if defined(OS_WIN)
#include "base/format_macros.h"
#include "base/path_service.h"
#include "base/files/file_path.h"
#include "components/crash/content/app/crash_export_thunks.h"
#include "components/crash/content/app/crash_switches.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/install_static/install_util.h"
#elif defined(OS_LINUX)
#include "components/crash/content/app/breakpad_linux.h"
#endif

#if defined(OS_MACOSX) || defined(OS_WIN)
#include "components/crash/content/app/crashpad.h"
#endif

MuonCrashReporterClient::MuonCrashReporterClient() {
}

MuonCrashReporterClient::~MuonCrashReporterClient() {}

#if defined(OS_POSIX) && !defined(OS_MACOSX)
void MuonCrashReporterClient::GetProductNameAndVersion(
    const char** product_name,
    const char** product_version) {
  auto command_line = base::CommandLine::ForCurrentProcess();
  std::string name = ATOM_PRODUCT_NAME;
  if (command_line->HasSwitch(atom::options::kAppName))
    name = command_line->GetSwitchValueASCII(atom::options::kAppName);

  std::string version = ATOM_VERSION_STRING;
  if (command_line->HasSwitch(atom::options::kAppVersion))
    version = command_line->GetSwitchValueASCII(atom::options::kAppVersion);

  *product_name = name.c_str();
  *product_version = version.c_str();
}

bool MuonCrashReporterClient::HandleCrashDump(const char* crashdump_filename) {
  return !MuonCrashReporterClient::GetCollectStatsConsent();
}

#endif

#if defined(OS_WIN) || defined(OS_MACOSX)
bool MuonCrashReporterClient::ShouldMonitorCrashHandlerExpensively() {
  return false;
}

bool MuonCrashReporterClient::ReportingIsEnforcedByPolicy(
    bool* breakpad_enabled) {
  return false;
}
#endif

bool MuonCrashReporterClient::GetCollectStatsConsent() {
  return GoogleUpdateSettings::GetCollectStatsConsent();
}

bool MuonCrashReporterClient::GetCollectStatsInSample() {
  return true;
}

//  static
void MuonCrashReporterClient::InitCrashReporting() {
  crash_reporter::InitializeCrashKeys();

  auto command_line = base::CommandLine::ForCurrentProcess();
  std::string process_type = command_line->GetSwitchValueASCII(
      ::switches::kProcessType);

#if !defined(OS_WIN)
  static MuonCrashReporterClient* instance = nullptr;

  if (instance)
    return;

  instance = new MuonCrashReporterClient();
  ANNOTATE_LEAKING_OBJECT_PTR(instance);

  crash_reporter::SetCrashReporterClient(instance);

#if defined(OS_MACOSX)
  const bool install_from_dmg_relauncher_process =
      process_type == switches::kRelauncherProcess &&
      command_line->HasSwitch(switches::kRelauncherProcessDMGDevice);
  const bool browser_process = process_type.empty();

  const bool initial_client =
      browser_process || install_from_dmg_relauncher_process;

  crash_reporter::InitializeCrashpad(initial_client, process_type);
#else
  breakpad::InitCrashReporter(process_type);
#endif
#endif  // !defined(OS_WIN)

  std::string version =
      command_line->GetSwitchValueASCII(atom::options::kAppVersion);
  if (!version.empty())
    SetVersionCrashValue(version);

  std::string channel =
        command_line->GetSwitchValueASCII(atom::options::kAppChannel);
  if (!channel.empty())
    SetChannelCrashValue(channel);

  static crash_reporter::CrashKeyString<32> muon_version_key("muon-version");
  muon_version_key.Set(ATOM_VERSION_STRING);
}

//  static
void MuonCrashReporterClient::SetCrashReportingEnabled(bool enabled) {
#if defined(OS_WIN)
  install_static::SetCollectStatsInSample(true);
  SetUploadConsent_ExportThunk(true);
#endif  // defined(OS_WIN)
  ChangeMetricsReportingStateWithReply(enabled,
      base::Bind(&SetCrashReportingEnabledForProcess));
}

// static
void MuonCrashReporterClient::SetChannelCrashValue(
    const std::string& value) {
  static crash_reporter::CrashKeyString<16> channel_key("channel");
  channel_key.Set(base::StringPiece(value));
}

//  static
void MuonCrashReporterClient::SetJavascriptInfoCrashValue(
    const std::string& value) {
  static crash_reporter::CrashKeyString<1024> javascript_info_key("javascript-info");
  javascript_info_key.Set(base::StringPiece(value));
}

// static
void MuonCrashReporterClient::SetNodeEnvCrashValue(
    const std::string& value) {
  static crash_reporter::CrashKeyString<32> node_env_key("node-env");
  node_env_key.Set(base::StringPiece(value));
}

// static
void MuonCrashReporterClient::SetVersionCrashValue(
    const std::string& value) {
  static crash_reporter::CrashKeyString<32> version_key("_version");
  version_key.Set(base::StringPiece(value));
}

//  static
void MuonCrashReporterClient::SetCrashReportingEnabledForProcess(bool enabled) {
  if (enabled) {
    InitCrashReporting();
  }
}

//  static
void MuonCrashReporterClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line) {
#if defined(OS_POSIX) && !defined(OS_MACOSX)
  if (breakpad::IsCrashReporterEnabled()) {
    command_line->AppendSwitch(switches::kEnableCrashReporter);
  }
#endif
}

//  static
void MuonCrashReporterClient::InitForProcess() {
  auto command_line = base::CommandLine::ForCurrentProcess();
  std::string process_type = command_line->GetSwitchValueASCII(
      ::switches::kProcessType);

  if (process_type.empty()) {
    return;
  }

#if defined(OS_POSIX) && !defined(OS_MACOSX)
  if (process_type == service_manager::switches::kZygoteProcess)
    return;
#endif

  InitCrashReporting();
}
