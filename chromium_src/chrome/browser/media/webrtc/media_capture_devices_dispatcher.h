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
#include "base/memory/singleton.h"
#include "base/observer_list.h"
#include "content/public/browser/media_observer.h"
#include "content/public/common/media_stream_request.h"

class MediaStreamCaptureIndicator;

// This singleton is used to receive updates about media events from the content
// layer.
class MediaCaptureDevicesDispatcher {
 public:
  class Observer {
   public:
    // Handle an information update consisting of a up-to-date audio capture
    // device lists. This happens when a microphone is plugged in or unplugged.
    virtual void OnUpdateAudioDevices(
        const content::MediaStreamDevices& devices) {}

    // Handle an information update consisting of a up-to-date video capture
    // device lists. This happens when a camera is plugged in or unplugged.
    virtual void OnUpdateVideoDevices(
        const content::MediaStreamDevices& devices) {}

    // Handle an information update related to a media stream request.
    virtual void OnRequestUpdate(
        int render_process_id,
        int render_frame_id,
        content::MediaStreamType stream_type,
        const content::MediaRequestState state) {}

    // Handle an information update that a new stream is being created.
    virtual void OnCreatingAudioStream(int render_process_id,
                                       int render_frame_id) {}

    virtual ~Observer() {}
  };

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
