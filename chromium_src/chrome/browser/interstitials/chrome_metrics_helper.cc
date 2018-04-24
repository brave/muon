// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/interstitials/chrome_metrics_helper.h"

#include "chrome/browser/ssl/captive_portal_metrics_recorder.h"
#include "content/public/browser/web_contents.h"

ChromeMetricsHelper::ChromeMetricsHelper(
    content::WebContents* web_contents,
    const GURL& request_url,
    const security_interstitials::MetricsHelper::ReportDetails settings)
    : security_interstitials::MetricsHelper(
          request_url,
          settings,
          nullptr) {}

ChromeMetricsHelper::~ChromeMetricsHelper() {}

void ChromeMetricsHelper::RecordExtraShutdownMetrics() {}
