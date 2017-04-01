// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_
#define CHROME_BROWSER_MEDIA_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_

#include <deque>
#include <list>
#include <map>
#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/scoped_vector.h"
#include "base/memory/singleton.h"
#include "base/observer_list.h"

class MediaStreamCaptureIndicator;

// This singleton is used to receive updates about media events from the content
// layer.
class MediaCaptureDevicesDispatcher {
 public:
  static MediaCaptureDevicesDispatcher* GetInstance();

  // Return true if there is any ongoing insecured capturing. The capturing is
  // deemed secure if all connected video sinks are reported secure and the
  // extension is trusted.
  bool IsInsecureCapturingInProgress(int render_process_id,
                                     int render_frame_id);

  scoped_refptr<MediaStreamCaptureIndicator> GetMediaStreamCaptureIndicator();
 private:
  friend struct base::DefaultSingletonTraits<MediaCaptureDevicesDispatcher>;

  MediaCaptureDevicesDispatcher();
  ~MediaCaptureDevicesDispatcher();

  scoped_refptr<MediaStreamCaptureIndicator> media_stream_capture_indicator_;

  DISALLOW_COPY_AND_ASSIGN(MediaCaptureDevicesDispatcher);
};

#endif  // CHROME_BROWSER_MEDIA_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_
