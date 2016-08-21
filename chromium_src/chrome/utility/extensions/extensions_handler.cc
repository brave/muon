// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/extensions/extensions_handler.h"

#include <utility>

#include "base/command_line.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "chrome/common/chrome_utility_messages.h"
#include "atom/utility/atom_content_utility_client.h"
#include "content/public/common/content_paths.h"
#include "content/public/utility/utility_thread.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_l10n_util.h"
#include "extensions/common/extension_utility_messages.h"
#include "extensions/utility/unpacker.h"
#include "ui/base/ui_base_switches.h"

namespace extensions {

bool Send(IPC::Message* message) {
  return content::UtilityThread::Get()->Send(message);
}

void ReleaseProcessIfNeeded() {
  content::UtilityThread::Get()->ReleaseProcessIfNeeded();
}

ExtensionsHandler::ExtensionsHandler(atom::AtomContentUtilityClient* utility_client)
    : utility_client_(utility_client) {
}

ExtensionsHandler::~ExtensionsHandler() {
}

// static
void ExtensionsHandler::PreSandboxStartup() {
}

bool ExtensionsHandler::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ExtensionsHandler, message)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled || utility_handler_.OnMessageReceived(message);
}

}  // namespace extensions
