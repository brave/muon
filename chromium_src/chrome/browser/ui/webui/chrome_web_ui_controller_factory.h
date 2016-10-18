// Copyright 2016 Chrome authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROME_WEB_UI_CONTROLLER_FACTORY_H_
#define CHROME_BROWSER_UI_WEBUI_CHROME_WEB_UI_CONTROLLER_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller_factory.h"
#include "ui/base/layout.h"

class ChromeWebUIControllerFactory : public content::WebUIControllerFactory {
 public:
  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) const override;
  bool UseWebUIForURL(content::BrowserContext* browser_context,
                      const GURL& url) const override;
  bool UseWebUIBindingsForURL(content::BrowserContext* browser_context,
                              const GURL& url) const override;
  content::WebUIController* CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) const override;

  static ChromeWebUIControllerFactory* GetInstance();

 protected:
  ChromeWebUIControllerFactory();
  ~ChromeWebUIControllerFactory() override;

 private:
  friend struct base::DefaultSingletonTraits<ChromeWebUIControllerFactory>;

  DISALLOW_COPY_AND_ASSIGN(ChromeWebUIControllerFactory);
};

#endif  // CHROME_BROWSER_UI_WEBUI_CHROME_WEB_UI_CONTROLLER_FACTORY_H_


