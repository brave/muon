#include "browser/inspectable_web_contents.h"

#include <memory>

#include "browser/inspectable_web_contents_impl.h"

namespace brightray {

InspectableWebContents* InspectableWebContents::Create(
    const content::WebContents::CreateParams& create_params) {
  std::unique_ptr<content::WebContents> contents =
      content::WebContents::Create(create_params);
  return Create(contents.release());
}

InspectableWebContents* InspectableWebContents::Create(
    content::WebContents* web_contents) {
  return new InspectableWebContentsImpl(web_contents);
}

}  // namespace brightray
