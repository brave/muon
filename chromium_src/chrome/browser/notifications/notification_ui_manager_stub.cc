// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_ui_manager_stub.h"

// static
NotificationUIManager* NotificationUIManagerStub::Create() {
  return new NotificationUIManagerStub();
}

void NotificationUIManagerStub::StartShutdown() {}
