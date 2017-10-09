// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/devtools_manager_delegate.h"

#include "build/build_config.h"
#include "chrome/browser/devtools/devtools_network_protocol_handler.h"
#include "chrome/browser/devtools/devtools_protocol_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tab_contents/tab_contents_iterator.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/process_manager.h"

namespace brightray {

namespace {

bool GetExtensionInfo(content::WebContents* wc,
                      std::string* name,
                      std::string* type) {
  Profile* profile = Profile::FromBrowserContext(wc->GetBrowserContext());
  if (!profile)
    return false;
  const extensions::Extension* extension =
      extensions::ProcessManager::Get(profile)->GetExtensionForWebContents(wc);
  if (!extension)
    return false;
  extensions::ExtensionHost* extension_host =
      extensions::ProcessManager::Get(profile)->GetBackgroundHostForExtension(
          extension->id());
  if (extension_host && extension_host->host_contents() == wc) {
    *name = extension->name();
    *type = "background_page";
    return true;
  }
  return false;
}

}  // namespace

DevToolsManagerDelegate::DevToolsManagerDelegate()
    : handler_(new DevToolsNetworkProtocolHandler) {
}

DevToolsManagerDelegate::~DevToolsManagerDelegate() {
}

void DevToolsManagerDelegate::DevToolsAgentHostAttached(
    content::DevToolsAgentHost* agent_host) {
  handler_->DevToolsAgentStateChanged(agent_host, true);
}

void DevToolsManagerDelegate::DevToolsAgentHostDetached(
    content::DevToolsAgentHost* agent_host) {
  handler_->DevToolsAgentStateChanged(agent_host, false);
}

base::DictionaryValue* DevToolsManagerDelegate::HandleCommand(
    content::DevToolsAgentHost* agent_host,
    base::DictionaryValue* command) {
  return handler_->HandleCommand(agent_host, command);
}

std::string DevToolsManagerDelegate::GetTargetType(
    content::WebContents* web_contents) {
  for (TabContentsIterator it; !it.done(); it.Next()) {
    if (*it == web_contents)
      return content::DevToolsAgentHost::kTypePage;
  }

  std::string extension_name;
  std::string extension_type;
  if (!GetExtensionInfo(web_contents, &extension_name, &extension_type))
    return content::DevToolsAgentHost::kTypeOther;
  return extension_type;
}

std::string DevToolsManagerDelegate::GetTargetTitle(
    content::WebContents* web_contents) {
  std::string extension_name;
  std::string extension_type;
  if (!GetExtensionInfo(web_contents, &extension_name, &extension_type))
    return std::string();
  return extension_name;
}

}  // namespace brightray
