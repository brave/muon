// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/media_stream_capture_indicator.h"

MediaStreamCaptureIndicator::MediaStreamCaptureIndicator() {
}

MediaStreamCaptureIndicator::~MediaStreamCaptureIndicator() {
}

bool MediaStreamCaptureIndicator::IsCapturingUserMedia(
    content::WebContents* web_contents) const {
  return false;
}

bool MediaStreamCaptureIndicator::IsBeingMirrored(
    content::WebContents* web_contents) const {
  return false;
}
