// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "muon/app/muon_crash_reporter_client.h"

#include "atom/common/atom_version.h"
#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/debug/crash_logging.h"
#include "base/debug/leak_annotations.h"
#include "base/path_service.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/crash_keys.h"
#include "chrome/installer/util/google_update_settings.h"
#include "components/crash/content/app/crashpad.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/common/content_switches.h"

#if defined(OS_WIN)
#include "base/format_macros.h"
#include "base/path_service.h"
#include "base/files/file_path.h"
#include "components/crash/content/app/crash_switches.h"
#include "chrome/common/chrome_constants.h"
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
  if (command_line->HasSwitch("muon_product_name"))
    name = command_line->GetSwitchValueASCII("muon_product_name");

  std::string version = ATOM_VERSION_STRING;
  if (command_line->HasSwitch("muon_product_version"))
    version = command_line->GetSwitchValueASCII("muon_product_version");

  *product_name = name.c_str();
  *product_version = version.c_str();
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
  auto command_line = base::CommandLine::ForCurrentProcess();
#if !defined(OS_WIN)
  static MuonCrashReporterClient* instance = nullptr;

  if (instance)
    return;

  instance = new MuonCrashReporterClient();
  ANNOTATE_LEAKING_OBJECT_PTR(instance);
  crash_reporter::SetCrashReporterClient(instance);

  std::string process_type = command_line->GetSwitchValueASCII(
      ::switches::kProcessType);
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
  crash_keys::SetCrashKeysFromCommandLine(*command_line);
}

//  static
void MuonCrashReporterClient::SetCrashReportingEnabled(bool enabled) {
  ChangeMetricsReportingStateWithReply(enabled,
      base::Bind(&SetCrashReportingEnabledForProcess));
}

//  static
void MuonCrashReporterClient::SetCrashKeyValue(const std::string& key,
                                                const std::string& value) {
  base::debug::SetCrashKeyValue(
      base::StringPiece(key), base::StringPiece(value));
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
#if defined(OS_WIN)
  return;
#else
  auto command_line = base::CommandLine::ForCurrentProcess();
  std::string process_type = command_line->GetSwitchValueASCII(
      ::switches::kProcessType);

  if (process_type.empty()) {
    return;
  }

#if defined(OS_POSIX) && !defined(OS_MACOSX)
  if (process_type == switches::kZygoteProcess)
    return;
#endif

  InitCrashReporting();
#endif  // defined(OS_WIN)
}
