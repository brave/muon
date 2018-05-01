#include "chrome/browser/ui/permission_bubble/permission_prompt.h"

// Create and display a platform specific prompt.
static std::unique_ptr<PermissionPrompt> Create(
    content::WebContents* web_contents,
    PermissionPrompt::Delegate* delegate) {
  return nullptr;
}
