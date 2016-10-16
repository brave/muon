// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_content_client.h"

#include <string>
#include <vector>

#include "atom/common/atom_version.h"
#include "atom/common/chrome_version.h"
#include "atom/common/options_switches.h"
#include "atom/common/pepper_flash_util.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/pepper_plugin_info.h"
#include "content/public/common/user_agent.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/url_constants.h"

#if defined(ENABLE_EXTENSIONS)
#include "content/public/common/url_constants.h"
#include "extensions/common/constants.h"
#endif

namespace atom {

namespace {
void ConvertStringWithSeparatorToVector(std::vector<std::string>* vec,
                                        const char* separator,
                                        const char* cmd_switch) {
  auto command_line = base::CommandLine::ForCurrentProcess();
  auto string_with_separator = command_line->GetSwitchValueASCII(cmd_switch);
  if (!string_with_separator.empty())
    *vec = base::SplitString(string_with_separator, separator,
                             base::TRIM_WHITESPACE,
                             base::SPLIT_WANT_NONEMPTY);
}

}  // namespace


AtomContentClient::AtomContentClient() {
}

AtomContentClient::~AtomContentClient() {
}

std::string AtomContentClient::GetProduct() const {
  return "Chrome/" CHROME_VERSION_STRING;
}

std::string AtomContentClient::GetUserAgent() const {
  return content::BuildUserAgentFromProduct(
      "Chrome/" CHROME_VERSION_STRING " "
      ATOM_PRODUCT_NAME "/" ATOM_VERSION_STRING);
}

base::string16 AtomContentClient::GetLocalizedString(int message_id) const {
  return l10n_util::GetStringUTF16(message_id);
}

void AtomContentClient::AddAdditionalSchemes(
    std::vector<url::SchemeWithType>* standard_schemes,
    std::vector<url::SchemeWithType>* referrer_schemes,
    std::vector<std::string>* savable_schemes) {
  standard_schemes->push_back({"chrome-extension", url::SCHEME_WITHOUT_PORT});
}

void AtomContentClient::AddSecureSchemesAndOrigins(
    std::set<std::string>* schemes,
    std::set<GURL>* origins) {
#if defined(ENABLE_EXTENSIONS)
  schemes->insert(content::kChromeUIScheme);
  schemes->insert(extensions::kExtensionScheme);
  schemes->insert(extensions::kExtensionResourceScheme);
#endif
}

void AtomContentClient::AddPepperPlugins(
    std::vector<content::PepperPluginInfo>* plugins) {
  AddPepperFlashFromCommandLine(plugins);
}

void AtomContentClient::AddServiceWorkerSchemes(
    std::set<std::string>* service_worker_schemes) {
  std::vector<std::string> schemes;
  ConvertStringWithSeparatorToVector(&schemes, ",",
                                     switches::kRegisterServiceWorkerSchemes);
  if (!schemes.empty()) {
    for (const std::string& scheme : schemes)
      service_worker_schemes->insert(scheme);
  }
  service_worker_schemes->insert(url::kFileScheme);
#if defined(ENABLE_EXTENSIONS)
  service_worker_schemes->insert(extensions::kExtensionScheme);
#endif
}

}  // namespace atom
