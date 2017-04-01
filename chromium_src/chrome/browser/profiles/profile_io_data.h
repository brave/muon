// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILES_PROFILE_IO_DATA_H_
#define CHROME_BROWSER_PROFILES_PROFILE_IO_DATA_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "build/build_config.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_interceptor.h"
#include "net/url_request/url_request_job_factory.h"

namespace net {
class URLRequestJobFactoryImpl;
}  // namespace net

// Conceptually speaking, the ProfileIOData represents data that lives on the IO
// thread that is owned by a Profile, such as, but not limited to, network
// objects like CookieMonster, HttpTransactionFactory, etc.  Profile owns
// ProfileIOData, but will make sure to delete it on the IO thread (except
// possibly in unit tests where there is no IO thread).
class ProfileIOData {
 public:
  // Returns true if |scheme| is handled in Chrome, or by default handlers in
  // net::URLRequest.
  static bool IsHandledProtocol(const std::string& scheme);

  // Returns true if |url| is handled in Chrome, or by default handlers in
  // net::URLRequest.
  static bool IsHandledURL(const GURL& url);

  // Utility to install additional WebUI handlers into the |job_factory|.
  // Ownership of the handlers is transfered from |protocol_handlers|
  // to the |job_factory|.
  static void InstallProtocolHandlers(
      net::URLRequestJobFactoryImpl* job_factory,
      content::ProtocolHandlerMap* protocol_handlers);

 private:
  DISALLOW_COPY_AND_ASSIGN(ProfileIOData);
};

#endif  // CHROME_BROWSER_PROFILES_PROFILE_IO_DATA_H_
