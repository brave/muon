// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MUON_APP_MUON_CRASH_REPORTER_CLIENT_H_
#define MUON_APP_MUON_CRASH_REPORTER_CLIENT_H_

#include "build/build_config.h"
#if defined(OS_WIN)
#include "chrome/app/chrome_crash_reporter_client_win.h"
#else
#include "chrome/app/chrome_crash_reporter_client.h"
#endif

namespace base {
class CommandLine;
class FilePath;
}

class PrefService;

class MuonCrashReporterClient : public ChromeCrashReporterClient {
 public:
  ~MuonCrashReporterClient() override;

  static void InitForProcess();
  static void InitCrashReporting();
  static void SetCrashReportingEnabled(bool enabled);
  static void SetCrashKeyValue(const std::string& key, const std::string& val);

#if defined(OS_POSIX) && !defined(OS_MACOSX)
  void GetProductNameAndVersion(const char** product_name,
                                const char** version) override;
#endif

#if defined(OS_WIN) || defined(OS_MACOSX)
  bool ReportingIsEnforcedByPolicy(bool* breakpad_enabled) override;
  bool ShouldMonitorCrashHandlerExpensively() override;
#endif

  bool GetCollectStatsConsent() override;
  bool GetCollectStatsInSample() override;

  static void AppendExtraCommandLineSwitches(
    base::CommandLine* command_line);

 private:
  MuonCrashReporterClient();
  static void SetCrashReportingEnabledForProcess(bool enabled);

  DISALLOW_COPY_AND_ASSIGN(MuonCrashReporterClient);
};

#endif  // MUON_APP_MUON_CRASH_REPORTER_CLIENT_H_
