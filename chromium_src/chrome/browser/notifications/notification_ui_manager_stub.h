// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_UI_MANAGER_STUB_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_UI_MANAGER_STUB_H_

#include "chrome/browser/notifications/notification_ui_manager.h"

class NotificationUIManagerStub : public NotificationUIManager {
 public:
  // Creates an initialized UI manager.
  static NotificationUIManager* Create();

  void Add(const Notification& notification, Profile* profile) override {}
  bool Update(const Notification& notification, Profile* profile) override {
    return false;
  }
  const Notification* FindById(const std::string& delegate_id,
                                       ProfileID profile_id) const override {
    return nullptr;
  }
  bool CancelById(const std::string& delegate_id,
                          ProfileID profile_id) override { return false; }
  std::set<std::string> GetAllIdsByProfile(ProfileID profile_id) override {
    return std::set<std::string>(); }
  bool CancelAllBySourceOrigin(const GURL& source_origin) override {
    return false;
  }
  bool CancelAllByProfile(ProfileID profile_id) override {
    return false;
  }
  void CancelAll() override {};
  void StartShutdown() override;

 protected:
  NotificationUIManagerStub() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationUIManagerStub);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_UI_MANAGER_STUB_H_
