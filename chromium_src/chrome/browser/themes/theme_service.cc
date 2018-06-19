#include "chrome/browser/themes/theme_service.h"

#include "chrome/browser/themes/custom_theme_supplier.h"
#include "chrome/browser/themes/theme_syncable_service.h"
#include "chrome/browser/ui/libgtkui/skia_utils_gtk.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_utils.h"

using ui::ResourceBundle;

class ThemeService::ThemeObserver {
};

const char ThemeService::kDefaultThemeID[] = "";

ThemeService::ThemeService()
    : ready_(false),
      rb_(ui::ResourceBundle::GetSharedInstance()),
      profile_(nullptr),
      installed_pending_load_id_(kDefaultThemeID),
      number_of_infobars_(0),
      original_theme_provider_(*this, false),
      incognito_theme_provider_(*this, true),
      weak_ptr_factory_(this) {}

ThemeService::~ThemeService() {
}

void ThemeService::Init(Profile* profile) {
}

void ThemeService::Observe(int type,
                           const content::NotificationSource& source,
                           const content::NotificationDetails& details) {}

void ThemeService::Shutdown() {}

void ThemeService::UseDefaultTheme() {}

void ThemeService::UseSystemTheme() {}

bool ThemeService::IsSystemThemeDistinctFromDefaultTheme() const {
  return false;
}

bool ThemeService::UsingDefaultTheme() const {
  return false;
}

bool ThemeService::UsingSystemTheme() const {
  return UsingDefaultTheme();
}

std::string ThemeService::GetThemeID() const {
  return ThemeService::kDefaultThemeID;
}

ThemeSyncableService* ThemeService::GetThemeSyncableService() const {
  return nullptr;
}

void ThemeService::SetCustomDefaultTheme(
    scoped_refptr<CustomThemeSupplier> theme_supplier) {
}

bool ThemeService::ShouldInitWithSystemTheme() const {
  return false;
}

SkColor ThemeService::GetDefaultColor(int id, bool incognito) const {
  return SkColorSetARGB(255, 233, 233, 233);
}

void ThemeService::ClearAllThemeData() {}

void ThemeService::FixInconsistentPreferencesIfNeeded() {}

void ThemeService::LoadThemePrefs() {}

void ThemeService::NotifyThemeChanged() {}

void ThemeService::FreePlatformCaches() {}

bool ThemeService::ShouldUseNativeFrame() const {
  return false;
}

void ThemeService::DoSetTheme(const extensions::Extension* extension,
                              bool suppress_infobar) {}

ThemeService::BrowserThemeProvider::BrowserThemeProvider(
    const ThemeService& theme_service,
    bool incognito)
    : theme_service_(theme_service), incognito_(incognito) {}

ThemeService::BrowserThemeProvider::~BrowserThemeProvider() {}

gfx::ImageSkia* ThemeService::BrowserThemeProvider::GetImageSkiaNamed(
    int id) const {
  return nullptr;
}

SkColor ThemeService::BrowserThemeProvider::GetColor(int id) const {
  return SkColorSetARGB(255, 233, 233, 233);
}

color_utils::HSL ThemeService::BrowserThemeProvider::GetTint(int id) const {
  return {0.0};
}

int ThemeService::BrowserThemeProvider::GetDisplayProperty(int id) const {
  return 0;
}

bool ThemeService::BrowserThemeProvider::ShouldUseNativeFrame() const {
  return false;
}

bool ThemeService::BrowserThemeProvider::HasCustomImage(int id) const {
  return false;
}

base::RefCountedMemory* ThemeService::BrowserThemeProvider::GetRawData(
    int id,
    ui::ScaleFactor scale_factor) const {
  return nullptr;
}
