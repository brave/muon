#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/themes/theme_service.h"

ThemeService* ThemeServiceFactory::GetForProfile(Profile* profile) {
  return new ThemeService;
}
