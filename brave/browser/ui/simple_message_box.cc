#include "chrome/browser/ui/simple_message_box.h"

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "build/build_config.h"
#include "ui/gfx/native_widget_types.h"

namespace chrome {

void ShowWarningMessageBox(gfx::NativeWindow parent,
                           const base::string16& title,
                           const base::string16& message) {
  //TODO(bridiver)
  // atom:Message...
}

}  // namespace chrome
