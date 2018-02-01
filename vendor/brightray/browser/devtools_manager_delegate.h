// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BROWSER_DEVTOOLS_MANAGER_DELEGATE_H_
#define BROWSER_DEVTOOLS_MANAGER_DELEGATE_H_

#include "base/macros.h"
#include "base/compiler_specific.h"
#include "content/browser/devtools/devtools_http_handler.h"
#include "content/public/browser/devtools_agent_host_observer.h"
#include "content/public/browser/devtools_manager_delegate.h"

namespace brightray {

class DevToolsManagerDelegate : public content::DevToolsManagerDelegate,
                                public content::DevToolsAgentHostObserver {
 public:
  static void StartHttpHandler();
  static void StopHttpHandler();

  DevToolsManagerDelegate();
  virtual ~DevToolsManagerDelegate();

 private:
  // DevToolsManagerDelegate implementation.
  void Inspect(content::DevToolsAgentHost* agent_host) override {}
  bool HandleCommand(content::DevToolsAgentHost* agent_host,
                     content::DevToolsAgentHostClient* client,
                     base::DictionaryValue* command) override;
  std::string GetTargetType(content::WebContents* web_contents) override {
    return std::string();
  }
  std::string GetTargetTitle(content::WebContents* web_contents) override {
    return std::string();
  }
  scoped_refptr<content::DevToolsAgentHost> CreateNewTarget(
    const GURL& url) override {return nullptr;}
  std::string GetDiscoveryPageHTML() override
    {return std::string();}
  std::string GetFrontendResource(const std::string& path) override
    {return std::string();}

  // content::DevToolsAgentHostObserver overrides.
  void DevToolsAgentHostAttached(
    content::DevToolsAgentHost* agent_host) override;
  void DevToolsAgentHostDetached(
    content::DevToolsAgentHost* agent_host) override;

  DISALLOW_COPY_AND_ASSIGN(DevToolsManagerDelegate);
};

}  // namespace brightray

#endif  // BROWSER_DEVTOOLS_MANAGER_DELEGATE_H_
