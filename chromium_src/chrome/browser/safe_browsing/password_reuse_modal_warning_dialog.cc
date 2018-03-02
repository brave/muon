#include "build/build_config.h"
#include "chrome/browser/safe_browsing/chrome_password_protection_service.h"
#include "content/public/browser/web_contents.h"

namespace safe_browsing {

void ShowPasswordReuseModalWarningDialog(
    content::WebContents* web_contents,
    ChromePasswordProtectionService* service,
    OnWarningDone done_callback) {}

}  // namespace safe_browsing
