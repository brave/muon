// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UTILITY_EXTENSIONS_EXTENSIONS_HANDLER_H_
#define CHROME_UTILITY_EXTENSIONS_EXTENSIONS_HANDLER_H_

#include <stdint.h>

#include "base/base64.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/utility/utility_message_handler.h"
#include "extensions/utility/utility_handler.h"

#if !defined(ENABLE_EXTENSIONS)
#error "Extensions must be enabled"
#endif

namespace atom {
  class AtomContentUtilityClient;
}

namespace extensions {

// Dispatches IPCs for Chrome extensions utility messages.
class ExtensionsHandler : public UtilityMessageHandler {
 public:
  explicit ExtensionsHandler(atom::AtomContentUtilityClient* utility_client);
  ~ExtensionsHandler() override;

  static void PreSandboxStartup();

  // UtilityMessageHandler:
  bool OnMessageReceived(const IPC::Message& message) override;

 private:

  UtilityHandler utility_handler_;

  // The client that owns this.
  atom::AtomContentUtilityClient* const utility_client_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionsHandler);
};

}  // namespace extensions

#endif  // CHROME_UTILITY_EXTENSIONS_EXTENSIONS_HANDLER_H_
