#include "chrome/browser/safe_browsing/chrome_password_protection_service.h"

namespace safe_browsing {

// static
bool ChromePasswordProtectionService::ShouldShowChangePasswordSettingUI(
    Profile* profile) {
  return false;
}

}  // namespace safe_browsing
