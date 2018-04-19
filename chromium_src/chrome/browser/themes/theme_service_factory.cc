#include "chrome/browser/themes/theme_service_factory.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/common/pref_names.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_factory.h"

// static
ThemeService* ThemeServiceFactory::GetForProfile(Profile* profile) {
  return new ThemeService;
}
