// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_WEBRTC_MEDIA_STREAM_CAPTURE_INDICATOR_H_
#define CHROME_BROWSER_MEDIA_WEBRTC_MEDIA_STREAM_CAPTURE_INDICATOR_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace content {
class WebContents;
}  // namespace content

class MediaStreamCaptureIndicator
    : public base::RefCountedThreadSafe<MediaStreamCaptureIndicator> {
 public:
  MediaStreamCaptureIndicator();

  bool IsCapturingUserMedia(content::WebContents* web_contents) const;
  bool IsBeingMirrored(content::WebContents* web_contents) const;
 private:
  friend class base::RefCountedThreadSafe<MediaStreamCaptureIndicator>;
  ~MediaStreamCaptureIndicator();

  DISALLOW_COPY_AND_ASSIGN(MediaStreamCaptureIndicator);
};

#endif  // CHROME_BROWSER_MEDIA_WEBRTC_MEDIA_STREAM_CAPTURE_INDICATOR_H_
