// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/password_manager/brave_password_manager_client.h"

#include <string>
#include <utility>

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/memory/singleton.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browsing_data/browsing_data_helper.h"
#include "chrome/browser/password_manager/password_store_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/autofill/password_generation_popup_controller_impl.h"
#include "chrome/browser/ui/passwords/passwords_client_ui_delegate.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/core/browser/password_generator.h"
#include "components/autofill/core/common/password_form.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/password_manager/content/browser/content_password_manager_driver.h"
#include "components/password_manager/content/browser/password_manager_internals_service_factory.h"
#include "components/password_manager/core/browser/browser_save_password_progress_logger.h"
#include "components/password_manager/core/browser/hsts_query.h"
#include "components/password_manager/core/browser/log_manager.h"
#include "components/password_manager/core/browser/log_receiver.h"
#include "components/password_manager/core/browser/password_bubble_experiment.h"
#include "components/password_manager/core/browser/password_form_manager.h"
#include "components/password_manager/core/browser/password_manager_internals_service.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/password_manager_util.h"
#include "components/password_manager/core/common/credential_manager_types.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/password_manager/sync/browser/password_sync_util.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/content/content_record_password_state.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/origin_util.h"
#include "extensions/features/features.h"
#include "google_apis/gaia/gaia_urls.h"
#include "net/base/url_util.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "third_party/re2/src/re2/re2.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/common/constants.h"
#endif

using password_manager::ContentPasswordManagerDriverFactory;
using password_manager::PasswordManagerInternalsService;
using password_manager::PasswordManagerMetricsRecorder;
using sessions::SerializedNavigationEntry;

// Shorten the name to spare line breaks. The code provides enough context
// already.
typedef autofill::SavePasswordProgressLogger Logger;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(BravePasswordManagerClient);

// static
void BravePasswordManagerClient::CreateForWebContentsWithAutofillClient(
    content::WebContents* contents,
    autofill::AutofillClient* autofill_client) {
  if (FromWebContents(contents))
    return;

  contents->SetUserData(
      UserDataKey(),
      base::MakeUnique<BravePasswordManagerClient>(contents, autofill_client));
}

// static
bool BravePasswordManagerClient::IsPossibleConfirmPasswordForm(
    const autofill::PasswordForm& form) {
  return form.new_password_element.empty() &&
    form.layout != autofill::PasswordForm::Layout::LAYOUT_LOGIN_AND_SIGNUP &&
    // https://chromium.googlesource.com/chromium/src/+/fdef64500de7e7cdfcc1a77ae7e82ad4a39d264f
    form.username_element == base::UTF8ToUTF16("anonymous_username");
}

BravePasswordManagerClient::BravePasswordManagerClient(
    content::WebContents* web_contents,
    autofill::AutofillClient* autofill_client)
    : content::WebContentsObserver(web_contents),
      profile_(Profile::FromBrowserContext(web_contents->GetBrowserContext())),
      password_manager_(this),
      password_reuse_detection_manager_(this),
      driver_factory_(nullptr),
      credential_manager_(this),
      password_manager_client_bindings_(web_contents, this),
      observer_(nullptr),
      credentials_filter_() {
  ContentPasswordManagerDriverFactory::CreateForWebContents(web_contents, this,
                                                            autofill_client);
  driver_factory_ =
      ContentPasswordManagerDriverFactory::FromWebContents(web_contents);
  log_manager_ = password_manager::LogManager::Create(
      password_manager::PasswordManagerInternalsServiceFactory::
          GetForBrowserContext(profile_),
      base::Bind(
          &ContentPasswordManagerDriverFactory::RequestSendLoggingAvailability,
          base::Unretained(driver_factory_)));

  saving_and_filling_passwords_enabled_.Init(
      password_manager::prefs::kCredentialsEnableService, GetPrefs());
  driver_factory_->RequestSendLoggingAvailability();
}

BravePasswordManagerClient::~BravePasswordManagerClient() {}

void BravePasswordManagerClient::Initialize(
  atom::api::WebContents* api_web_contents) {
  api_web_contents_ = api_web_contents;
}

void BravePasswordManagerClient::DidClickSave() {
  if (form_to_save_)
    form_to_save_->Save();
}

void BravePasswordManagerClient::DidClickNever() {
  if (form_to_save_)
    form_to_save_->OnNeverClicked();
}

void BravePasswordManagerClient::DidClickUpdate() {
  if (form_to_save_) {
    form_to_save_->Update(form_to_save_->pending_credentials());
  }
}

void BravePasswordManagerClient::DidClickNoUpdate() {
  if (form_to_save_)
    form_to_save_->OnNopeUpdateClicked();
}

bool BravePasswordManagerClient::IsPasswordManagementEnabledForCurrentPage()
    const {
  DCHECK(web_contents());
  PrefService* prefs = profile_->GetPrefs();
  DCHECK(prefs);
  if (!prefs->GetBoolean(password_manager::prefs::kCredentialsEnableService))
    return false;
  content::NavigationEntry* entry =
      web_contents()->GetController().GetLastCommittedEntry();
  bool is_enabled = false;
  if (!entry) {
    // TODO(gcasto): Determine if fix for crbug.com/388246 is relevant here.
    is_enabled = true;
  } else {
    // Do not fill nor save password when a user is signing in for sync. This
    // is because users need to remember their password if they are syncing as
    // this is effectively their master password.
    is_enabled =
        entry->GetURL().host_piece() != chrome::kChromeUIChromeSigninHost;
  }
  if (log_manager_->IsLoggingActive()) {
    password_manager::BrowserSavePasswordProgressLogger logger(
        log_manager_.get());
    logger.LogBoolean(
        Logger::STRING_PASSWORD_MANAGEMENT_ENABLED_FOR_CURRENT_PAGE,
        is_enabled);
  }
  return is_enabled;
}

bool BravePasswordManagerClient::IsSavingAndFillingEnabledForCurrentPage()
    const {
  // TODO(melandory): remove saving_and_filling_passwords_enabled_ check from
  // here once we decide to switch to new settings behavior for everyone.
  return *saving_and_filling_passwords_enabled_ && !IsIncognito() &&
         IsFillingEnabledForCurrentPage();
}

bool BravePasswordManagerClient::IsFillingEnabledForCurrentPage() const {
  return !DidLastPageLoadEncounterSSLErrors() &&
         IsPasswordManagementEnabledForCurrentPage();
}

void BravePasswordManagerClient::PostHSTSQueryForHost(
    const GURL& origin,
    const HSTSCallback& callback) const {
  password_manager::PostHSTSQueryForHostAndRequestContext(
      origin, base::WrapRefCounted(profile_->GetRequestContext()), callback);
}

bool BravePasswordManagerClient::OnCredentialManagerUsed() {
  return true;
}

bool BravePasswordManagerClient::PromptUserToSaveOrUpdatePassword(
    std::unique_ptr<password_manager::PasswordFormManager> form_to_save,
    bool update_password) {
  // Save password infobar and the password bubble prompts in case of
  // "webby" URLs and do not prompt in case of "non-webby" URLS (e.g. file://).
  if (!CanShowBubbleOnURL(web_contents()->GetLastCommittedURL()))
    return false;
  const autofill::PasswordForm *form = form_to_save->submitted_form();
  // Don't save password for confirmation page (ex. Trezor passphrase)
  if (IsPossibleConfirmPasswordForm(*form))
    return false;
  form_to_save_ = std::move(form_to_save);
  if (update_password) {
    api_web_contents_->Emit("update-password", form->username_value,
                            form->signon_realm);
  } else {
    if (form_to_save_->IsBlacklisted())
      return false;
    if (api_web_contents_) {
      api_web_contents_->Emit("save-password", form->username_value,
                              form->signon_realm);
    }
  }
  return true;
}

void BravePasswordManagerClient::ShowManualFallbackForSaving(
    std::unique_ptr<password_manager::PasswordFormManager> form_to_save,
    bool has_generated_password,
    bool is_update) {}

void BravePasswordManagerClient::HideManualFallbackForSaving() {}

bool BravePasswordManagerClient::PromptUserToChooseCredentials(
    std::vector<std::unique_ptr<autofill::PasswordForm>> local_forms,
    const GURL& origin,
    const CredentialsCallback& callback) {
  // Set up an intercept callback if the prompt is zero-clickable (e.g. just one
  // form provided).
  CredentialsCallback intercept =
      base::Bind(&BravePasswordManagerClient::OnCredentialsChosen,
                 base::Unretained(this), callback, local_forms.size() == 1);
  return true;
}

void BravePasswordManagerClient::OnCredentialsChosen(
    const CredentialsCallback& callback,
    bool one_local_credential,
    const autofill::PasswordForm* form) {
  callback.Run(form);
  // If a site gets back a credential some navigations are likely to occur. They
  // shouldn't trigger the autofill password manager.
  if (form)
    password_manager_.DropFormManagers();
  if (form && one_local_credential)
    PromptUserToEnableAutosigninIfNecessary();
}

void BravePasswordManagerClient::ForceSavePassword() {
  password_manager::ContentPasswordManagerDriver* driver =
      driver_factory_->GetDriverForFrame(web_contents()->GetFocusedFrame());
  driver->ForceSavePassword();
}

void BravePasswordManagerClient::GeneratePassword() {
  password_manager::ContentPasswordManagerDriver* driver =
      driver_factory_->GetDriverForFrame(web_contents()->GetFocusedFrame());
  driver->GeneratePassword();
}

void BravePasswordManagerClient::NotifyUserAutoSignin(
    std::vector<std::unique_ptr<autofill::PasswordForm>> local_forms,
    const GURL& origin) {
  DCHECK(!local_forms.empty());
  // If a site gets back a credential some navigations are likely to occur. They
  // shouldn't trigger the autofill password manager.
  password_manager_.DropFormManagers();
}

void BravePasswordManagerClient::NotifyUserCouldBeAutoSignedIn(
    std::unique_ptr<autofill::PasswordForm> form) {
  possible_auto_sign_in_ = std::move(form);
}

void BravePasswordManagerClient::NotifySuccessfulLoginWithExistingPassword(
    const autofill::PasswordForm& form) {
  if (!possible_auto_sign_in_)
    return;

  if (possible_auto_sign_in_->username_value == form.username_value &&
      possible_auto_sign_in_->password_value == form.password_value &&
      possible_auto_sign_in_->origin == form.origin) {
    PromptUserToEnableAutosigninIfNecessary();
  }
  possible_auto_sign_in_.reset();
}

void BravePasswordManagerClient::NotifyStorePasswordCalled() {
  // If a site stores a credential the autofill password manager shouldn't kick
  // in.
  password_manager_.DropFormManagers();
}

void BravePasswordManagerClient::AutomaticPasswordSave(
    std::unique_ptr<password_manager::PasswordFormManager> saved_form) {
}

void BravePasswordManagerClient::PasswordWasAutofilled(
    const std::map<base::string16, const autofill::PasswordForm*>& best_matches,
    const GURL& origin,
    const std::vector<const autofill::PasswordForm*>* federated_matches) const {
}

void BravePasswordManagerClient::HidePasswordGenerationPopup() {
}

PasswordManagerMetricsRecorder&
BravePasswordManagerClient::GetMetricsRecorder() {
  if (!metrics_recorder_) {
    metrics_recorder_.emplace(GetUkmRecorder(), GetUkmSourceId(),
                              GetMainFrameURL());
  }
  return metrics_recorder_.value();
}

void BravePasswordManagerClient::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() || !navigation_handle->HasCommitted())
    return;

  if (!navigation_handle->IsSameDocument())
    ukm_source_id_.reset();

  // From this point on, the ContentCredentialManager will service API calls in
  // the context of the new WebContents::GetLastCommittedURL, which may very
  // well be cross-origin. Disconnect existing client, and drop pending
  // requests.
  if (!navigation_handle->IsSameDocument())
    credential_manager_.DisconnectBinding();

  password_reuse_detection_manager_.DidNavigateMainFrame(GetMainFrameURL());
  // After some navigations RenderViewHost persists and just adding the observer
  // will cause multiple call of OnInputEvent. Since Widget API doesn't allow to
  // check whether the observer is already added, the observer is removed and
  // added again, to ensure that it is added only once.
  web_contents()->GetRenderViewHost()->GetWidget()->RemoveInputEventObserver(
      this);
  web_contents()->GetRenderViewHost()->GetWidget()->AddInputEventObserver(this);
}

void BravePasswordManagerClient::OnInputEvent(
    const blink::WebInputEvent& event) {
  if (event.GetType() != blink::WebInputEvent::kChar)
    return;
  const blink::WebKeyboardEvent& key_event =
      static_cast<const blink::WebKeyboardEvent&>(event);
  password_reuse_detection_manager_.OnKeyPressed(key_event.text);
}

PrefService* BravePasswordManagerClient::GetPrefs() {
  return profile_->GetPrefs();
}

password_manager::PasswordStore*
BravePasswordManagerClient::GetPasswordStore() const {
  // Always use EXPLICIT_ACCESS as the password manager checks IsIncognito
  // itself when it shouldn't access the PasswordStore.
  // TODO(gcasto): Is is safe to change this to
  // ServiceAccessType::IMPLICIT_ACCESS?
  return PasswordStoreFactory::GetForProfile(
             profile_, ServiceAccessType::EXPLICIT_ACCESS).get();
}

#if defined(SAFE_BROWSING_DB_LOCAL)
safe_browsing::PasswordProtectionService*
BravePasswordManagerClient::GetPasswordProtectionService() const {
  return nullptr;
}

void BravePasswordManagerClient::CheckSafeBrowsingReputation(
    const GURL& form_action,
    const GURL& frame_url) {}

void BravePasswordManagerClient::LogPasswordReuseDetectedEvent() {}

void BravePasswordManagerClient::CheckProtectedPasswordEntry(
    bool matches_sync_password,
    const std::vector<std::string>& matching_domains,
    bool password_field_exists) {}
#endif

ukm::UkmRecorder* BravePasswordManagerClient::GetUkmRecorder() {
  return ukm::UkmRecorder::Get();
}

ukm::SourceId BravePasswordManagerClient::GetUkmSourceId() {
  // TODO(crbug.com/732846): The UKM Source should be recycled (e.g. from the
  // web contents), once the UKM framework provides a mechanism for that.
  if (!ukm_source_id_) {
    ukm_source_id_ = ukm::UkmRecorder::GetNewSourceID();
    ukm::UkmRecorder* ukm_recorder = GetUkmRecorder();
    if (ukm_recorder)
      ukm_recorder->UpdateSourceURL(*ukm_source_id_, GetMainFrameURL());
  }
  return *ukm_source_id_;
}

password_manager::PasswordSyncState
BravePasswordManagerClient::GetPasswordSyncState() const {
  return password_manager_util::GetPasswordSyncState(nullptr);
}

bool BravePasswordManagerClient::WasLastNavigationHTTPError() const {
  DCHECK(web_contents());

  std::unique_ptr<password_manager::BrowserSavePasswordProgressLogger> logger;
  if (log_manager_->IsLoggingActive()) {
    logger.reset(new password_manager::BrowserSavePasswordProgressLogger(
        log_manager_.get()));
    logger->LogMessage(
        Logger::STRING_WAS_LAST_NAVIGATION_HTTP_ERROR_METHOD);
  }

  content::NavigationEntry* entry =
      web_contents()->GetController().GetVisibleEntry();
  if (!entry)
    return false;
  int http_status_code = entry->GetHttpStatusCode();

  if (logger)
    logger->LogNumber(Logger::STRING_HTTP_STATUS_CODE, http_status_code);

  if (http_status_code >= 400 && http_status_code < 600)
    return true;
  return false;
}

bool BravePasswordManagerClient::DidLastPageLoadEncounterSSLErrors() const {
  content::NavigationEntry* entry =
      web_contents()->GetController().GetLastCommittedEntry();
  bool ssl_errors = true;
  if (!entry) {
    ssl_errors = false;
  } else {
    ssl_errors = net::IsCertStatusError(entry->GetSSL().cert_status);
  }
  if (log_manager_->IsLoggingActive()) {
    password_manager::BrowserSavePasswordProgressLogger logger(
        log_manager_.get());
    logger.LogBoolean(Logger::STRING_SSL_ERRORS_PRESENT, ssl_errors);
  }
  return ssl_errors;
}

bool BravePasswordManagerClient::IsIncognito() const {
  return web_contents()->GetBrowserContext()->IsOffTheRecord();
}

const password_manager::PasswordManager*
BravePasswordManagerClient::GetPasswordManager() const {
  return &password_manager_;
}

autofill::AutofillManager*
BravePasswordManagerClient::GetAutofillManagerForMainFrame() {
  autofill::ContentAutofillDriverFactory* factory =
      autofill::ContentAutofillDriverFactory::FromWebContents(web_contents());
  return factory
             ? factory->DriverForFrame(web_contents()->GetMainFrame())
                   ->autofill_manager()
             : nullptr;
}

void BravePasswordManagerClient::SetTestObserver(
    autofill::PasswordGenerationPopupObserver* observer) {
  observer_ = observer;
}

void BravePasswordManagerClient::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  // Logging has no sense on WebUI sites.
  log_manager_->SetSuspended(web_contents()->GetWebUI() != nullptr);
}

gfx::RectF BravePasswordManagerClient::GetBoundsInScreenSpace(
    const gfx::RectF& bounds) {
  gfx::Rect client_area = web_contents()->GetContainerBounds();
  return bounds + client_area.OffsetFromOrigin();
}

void BravePasswordManagerClient::ShowPasswordGenerationPopup(
    const gfx::RectF& bounds,
    int max_length,
    const base::string16& generation_element,
    bool is_manually_triggered,
    const autofill::PasswordForm& form) {
  // TODO(gcasto): Validate data in PasswordForm.

  auto* driver = driver_factory_->GetDriverForFrame(
      password_manager_client_bindings_.GetCurrentTargetFrame());
  password_manager_.SetGenerationElementAndReasonForForm(
      driver, form, generation_element, is_manually_triggered);
}

void BravePasswordManagerClient::ShowPasswordEditingPopup(
    const gfx::RectF& bounds,
    const autofill::PasswordForm& form) {
}

void BravePasswordManagerClient::PromptUserToEnableAutosigninIfNecessary() {
  if (!password_bubble_experiment::ShouldShowAutoSignInPromptFirstRunExperience(
          GetPrefs()) ||
      !GetPrefs()->GetBoolean(
          password_manager::prefs::kCredentialsEnableAutosignin) ||
      IsIncognito())
    return;
}

void BravePasswordManagerClient::GenerationAvailableForForm(
    const autofill::PasswordForm& form) {
  password_manager_.GenerationAvailableForForm(form);
}

const GURL& BravePasswordManagerClient::GetMainFrameURL() const {
  return web_contents()->GetVisibleURL();
}

bool BravePasswordManagerClient::IsMainFrameSecure() const {
  return content::IsOriginSecure(web_contents()->GetVisibleURL());
}

const GURL& BravePasswordManagerClient::GetLastCommittedEntryURL() const {
  DCHECK(web_contents());
  content::NavigationEntry* entry =
      web_contents()->GetController().GetLastCommittedEntry();
  if (!entry)
    return GURL::EmptyGURL();

  return entry->GetURL();
}

// static
bool BravePasswordManagerClient::ShouldAnnotateNavigationEntries(
    Profile* profile) {
  return true;
}

void BravePasswordManagerClient::AnnotateNavigationEntry(
    bool has_password_field) {
  if (!ShouldAnnotateNavigationEntries(profile_))
    return;

  content::NavigationEntry* entry =
      web_contents()->GetController().GetLastCommittedEntry();
  if (!entry)
    return;

  SerializedNavigationEntry::PasswordState old_state =
      sessions::GetPasswordStateFromNavigation(*entry);

  SerializedNavigationEntry::PasswordState new_state =
      (has_password_field ? SerializedNavigationEntry::HAS_PASSWORD_FIELD
                          : SerializedNavigationEntry::NO_PASSWORD_FIELD);

  if (new_state > old_state) {
    SetPasswordStateInNavigation(new_state, entry);
  }
}

const password_manager::CredentialsFilter*
BravePasswordManagerClient::GetStoreResultFilter() const {
  return &credentials_filter_;
}

const password_manager::LogManager* BravePasswordManagerClient::GetLogManager()
    const {
  return log_manager_.get();
}

// static
void BravePasswordManagerClient::BindCredentialManager(
    password_manager::mojom::CredentialManagerAssociatedRequest request,
    content::RenderFrameHost* render_frame_host) {
  // Only valid for the main frame.
  if (render_frame_host->GetParent())
    return;

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  DCHECK(web_contents);

  // Only valid for the currently committed RenderFrameHost, and not, e.g. old
  // zombie RFH's being swapped out following cross-origin navigations.
  if (web_contents->GetMainFrame() != render_frame_host)
    return;

  BravePasswordManagerClient* instance =
      BravePasswordManagerClient::FromWebContents(web_contents);

  // Try to bind to the driver, but if driver is not available for this render
  // frame host, the request will be just dropped. This will cause the message
  // pipe to be closed, which will raise a connection error on the peer side.
  if (!instance)
    return;

  instance->credential_manager_.BindRequest(std::move(request));
}

// static
bool BravePasswordManagerClient::CanShowBubbleOnURL(const GURL& url) {
  std::string scheme = url.scheme();
  return (content::ChildProcessSecurityPolicy::GetInstance()->IsWebSafeScheme(
              scheme) &&
#if BUILDFLAG(ENABLE_EXTENSIONS)
          scheme != extensions::kExtensionScheme &&
#endif
          scheme != content::kChromeDevToolsScheme);
}
