// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_process_manager_delegate.h"

#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/browser.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/process_manager.h"
#include "extensions/browser/process_manager_factory.h"
#include "extensions/common/one_shot_event.h"

namespace extensions {

AtomProcessManagerDelegate::AtomProcessManagerDelegate() {
  registrar_.Add(this,
                 chrome::NOTIFICATION_PROFILE_CREATED,
                 content::NotificationService::AllSources());
  registrar_.Add(this,
                 chrome::NOTIFICATION_PROFILE_DESTROYED,
                 content::NotificationService::AllSources());
}

AtomProcessManagerDelegate::~AtomProcessManagerDelegate() {
}

bool AtomProcessManagerDelegate::AreBackgroundPagesAllowedForContext(
    content::BrowserContext* context) const {
  return true;
}

bool AtomProcessManagerDelegate::DeferCreatingStartupBackgroundHosts(
    content::BrowserContext* context) const {
  return !atom::Browser::Get()->is_ready();
}

void AtomProcessManagerDelegate::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_PROFILE_CREATED: {
      Profile* profile =
        content::Source<Profile>(source).ptr();
      OnProfileCreated(profile);
      break;
    }
    case chrome::NOTIFICATION_PROFILE_DESTROYED: {
      Profile* profile =
        content::Source<Profile>(source).ptr();
      OnProfileDestroyed(profile);
      break;
    }
    default:
      NOTREACHED();
  }
}

void AtomProcessManagerDelegate::OnProfileCreated(
                                        Profile* profile) {
  // Profiles are handled by their original profile.
  if (profile->GetOriginalProfile() != profile)
    return;

  // The profile can be created before the extension system is ready.
  if (!ExtensionSystem::Get(profile)->ready().is_signaled())
    return;

  // The profile might have been initialized asynchronously (in
  // parallel with extension system startup). Now that initialization is
  // complete the ProcessManager can load deferred background pages.
  ProcessManager::Get(profile)->MaybeCreateStartupBackgroundHosts();
}

void AtomProcessManagerDelegate::OnProfileDestroyed(
                                        Profile* profile) {
  ProcessManager* manager =
      ProcessManagerFactory::GetForBrowserContextIfExists(profile);
  if (manager) {
    manager->CloseBackgroundHosts();
  }
}

}  // namespace extensions
