// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_security_state_model_client.h"

#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/origin_util.h"
#include "net/cert/x509_certificate.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(atom::AtomSecurityStateModelClient);

using security_state::SecurityStateModel;

namespace atom {

namespace {

SecurityStateModel::SecurityLevel GetSecurityLevelForSecurityStyle(
    blink::WebSecurityStyle style) {
  switch (style) {
    case blink::WebSecurityStyleUnknown:
      return SecurityStateModel::NONE;
    case blink::WebSecurityStyleUnauthenticated:
      return SecurityStateModel::NONE;
    case blink::WebSecurityStyleAuthenticationBroken:
      return SecurityStateModel::DANGEROUS;
    case blink::WebSecurityStyleWarning:
      return SecurityStateModel::SECURITY_WARNING;
    case blink::WebSecurityStyleAuthenticated:
      return SecurityStateModel::SECURE;
  }
  return SecurityStateModel::NONE;
}

}  // namespace

AtomSecurityStateModelClient::AtomSecurityStateModelClient(
    content::WebContents* web_contents)
    : web_contents_(web_contents),
      security_state_model_(new SecurityStateModel()) {
  security_state_model_->SetClient(this);
}

AtomSecurityStateModelClient::~AtomSecurityStateModelClient() {
}

void AtomSecurityStateModelClient::GetSecurityInfo(
  SecurityStateModel::SecurityInfo* result) const {
  security_state_model_->GetSecurityInfo(result);
}

bool AtomSecurityStateModelClient::RetrieveCert(
    scoped_refptr<net::X509Certificate>* cert) {
  content::NavigationEntry* entry =
      web_contents_->GetController().GetVisibleEntry();
  if (!entry || !entry->GetSSL().certificate)
    return false;
  *cert = entry->GetSSL().certificate;
  return true;
}

bool AtomSecurityStateModelClient::UsedPolicyInstalledCertificate() {
  return false;
}

bool AtomSecurityStateModelClient::IsOriginSecure(const GURL& url) {
  return content::IsOriginSecure(url);
}

void AtomSecurityStateModelClient::GetVisibleSecurityState(
    SecurityStateModel::VisibleSecurityState* state) {
  content::NavigationEntry* entry =
      web_contents_->GetController().GetVisibleEntry();
  if (!entry ||
      entry->GetSSL().security_style == blink::WebSecurityStyleUnknown) {
    *state = SecurityStateModel::VisibleSecurityState();
    return;
  }

  state->connection_info_initialized = true;
  state->url = entry->GetURL();
  const content::SSLStatus& ssl = entry->GetSSL();
  state->initial_security_level =
      GetSecurityLevelForSecurityStyle(ssl.security_style);
  state->certificate = ssl.certificate;
  state->cert_status = ssl.cert_status;
  state->connection_status = ssl.connection_status;
  state->security_bits = ssl.security_bits;
  state->pkp_bypassed = ssl.pkp_bypassed;
  state->sct_verify_statuses.clear();
  state->sct_verify_statuses.insert(state->sct_verify_statuses.begin(),
                                    ssl.sct_statuses.begin(),
                                    ssl.sct_statuses.end());
  state->displayed_mixed_content =
      !!(ssl.content_status & content::SSLStatus::DISPLAYED_INSECURE_CONTENT);
  state->ran_mixed_content =
      !!(ssl.content_status & content::SSLStatus::RAN_INSECURE_CONTENT);
  state->displayed_content_with_cert_errors =
      !!(ssl.content_status &
         content::SSLStatus::DISPLAYED_CONTENT_WITH_CERT_ERRORS);
  state->ran_content_with_cert_errors =
      !!(ssl.content_status & content::SSLStatus::RAN_CONTENT_WITH_CERT_ERRORS);
}

}  // namespace atom
