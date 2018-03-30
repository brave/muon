// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/interstitials/chrome_metrics_helper.h"

#include "chrome/common/features.h"
#include "content/public/browser/web_contents.h"
#include "extensions/features/features.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/api/experience_sampling_private/experience_sampling.h"
#endif

#if BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION)
#include "chrome/browser/ssl/captive_portal_metrics_recorder.h"
#endif

ChromeMetricsHelper::ChromeMetricsHelper(
    content::WebContents* web_contents,
    const GURL& request_url,
    const security_interstitials::MetricsHelper::ReportDetails settings,
    const std::string& sampling_event_name)
    : security_interstitials::MetricsHelper(
          request_url,
          settings,
          nullptr),
#if BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION) || BUILDFLAG(ENABLE_EXTENSIONS)
      web_contents_(web_contents),
#endif
      request_url_(request_url),
      sampling_event_name_(sampling_event_name) {
  DCHECK(!sampling_event_name_.empty());
}

ChromeMetricsHelper::~ChromeMetricsHelper() {}

void ChromeMetricsHelper::RecordExtraShutdownMetrics() {}

void ChromeMetricsHelper::RecordExtraUserDecisionMetrics(
    security_interstitials::MetricsHelper::Decision decision) {}

void ChromeMetricsHelper::RecordExtraUserInteractionMetrics(
    security_interstitials::MetricsHelper::Interaction interaction) {}
