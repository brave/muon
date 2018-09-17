#include "ui/base/ui_base_features.h"

#include "ui/base/ui_base_switches_util.h"

#if defined(OS_MACOSX) && !BUILDFLAG(MAC_VIEWS_BROWSER)

namespace features {

bool IsViewsBrowserCocoa() {
  return false;
}

}  // namespace features

namespace views_mode_controller {

bool IsViewsBrowserCocoa() {
  return features::IsViewsBrowserCocoa();
}

}  // namespace views_mode_controller

#endif
