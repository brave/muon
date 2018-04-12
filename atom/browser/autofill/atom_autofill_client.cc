// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/autofill/atom_autofill_client.h"

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/autofill/personal_data_manager_factory.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_browser_context.h"
#include "brave/browser/password_manager/brave_password_manager_client.h"
#include "chrome/browser/browser_process.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/password_manager/content/browser/content_password_manager_driver.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/common/origin_util.h"
#include "native_mate/dictionary.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(autofill::AtomAutofillClient);

namespace mate {

template<>
struct Converter<autofill::Suggestion> {
  static v8::Local<v8::Value> ToV8(
    v8::Isolate* isolate, autofill::Suggestion val) {
    mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
    dict.Set("value", val.value);
    dict.Set("label", val.label);
    dict.Set("icon", val.icon);
    dict.Set("frontend_id", val.frontend_id);
    return dict.GetHandle();
  }
};
}  // namespace mate

namespace autofill {

AtomAutofillClient::AtomAutofillClient(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      api_web_contents_(nullptr) {
  DCHECK(web_contents);
}

AtomAutofillClient::~AtomAutofillClient() {
}

void AtomAutofillClient::Initialize(atom::api::WebContents* api_web_contents) {
  api_web_contents_ = api_web_contents;
}

void AtomAutofillClient::DidAcceptSuggestion(const std::string& value,
                                             int frontend_id,
                                             int index) {
  if (delegate_) {
    delegate_->DidAcceptSuggestion(base::UTF8ToUTF16(value), frontend_id,
                                   index);
  }
}

PersonalDataManager* AtomAutofillClient::GetPersonalDataManager() {
  content::BrowserContext* context = web_contents()->GetBrowserContext();
  return PersonalDataManagerFactory::GetForBrowserContext(context);
}

autofill::SaveCardBubbleController*
AtomAutofillClient::GetSaveCardBubbleController() {
  return nullptr;
}

scoped_refptr<AutofillWebDataService> AtomAutofillClient::GetDatabase() {
  content::BrowserContext* context = web_contents()->GetBrowserContext();
  return static_cast<brave::BraveBrowserContext*>(context)
                ->GetAutofillWebdataService();
}

PrefService* AtomAutofillClient::GetPrefs() {
  return user_prefs::UserPrefs::Get(web_contents()->GetBrowserContext());
}

syncer::SyncService* AtomAutofillClient::GetSyncService() {
  return nullptr;
}

identity::IdentityManager* AtomAutofillClient::GetIdentityManager() {
  return nullptr;
}

ukm::UkmRecorder* AtomAutofillClient::GetUkmRecorder() {
  return ukm::UkmRecorder::Get();
}

AddressNormalizer* AtomAutofillClient::GetAddressNormalizer() {
  return nullptr;
}

void AtomAutofillClient::ShowAutofillSettings() {
  if (api_web_contents_) {
    api_web_contents_->Emit("show-autofill-settings");
  }
}

void AtomAutofillClient::ShowUnmaskPrompt(
    const CreditCard& card,
    UnmaskCardReason reason,
    base::WeakPtr<CardUnmaskDelegate> delegate) {
}

void AtomAutofillClient::OnUnmaskVerificationResult(
    PaymentsRpcResult result) {
}

void AtomAutofillClient::ConfirmSaveCreditCardLocally(
    const CreditCard& card,
    const base::Closure& callback) {
}

void AtomAutofillClient::ConfirmSaveCreditCardToCloud(
    const CreditCard& card,
    std::unique_ptr<base::DictionaryValue> legal_message,
    bool should_cvc_be_requested,
    const base::Closure& callback) {
}

void AtomAutofillClient::LoadRiskData(
    const base::Callback<void(const std::string&)>& callback) {
}

bool AtomAutofillClient::HasCreditCardScanFeature() {
  return false;
}

void AtomAutofillClient::ScanCreditCard(
    const CreditCardScanCallback& callback) {
}

void AtomAutofillClient::ShowAutofillPopup(
    const gfx::RectF& element_bounds,
    base::i18n::TextDirection text_direction,
    const std::vector<autofill::Suggestion>& suggestions,
    base::WeakPtr<AutofillPopupDelegate> delegate) {
  delegate_ = delegate;
  if (api_web_contents_) {
    v8::Locker locker(api_web_contents_->isolate());
    v8::HandleScope handle_scope(api_web_contents_->isolate());
    mate::Dictionary rect =
      mate::Dictionary::CreateEmpty(api_web_contents_->isolate());
    gfx::Rect client_area = web_contents()->GetContainerBounds();
    rect.Set("x", element_bounds.x());
    rect.Set("y", element_bounds.y());
    rect.Set("width", element_bounds.width());
    rect.Set("height", element_bounds.height());
    rect.Set("clientWidth", client_area.width());
    rect.Set("clientHeight", client_area.height());

    // Give app a chance to cancel popup.
    if (!api_web_contents_->Emit("show-autofill-popup",
                            suggestions,
                            rect))
      delegate->OnPopupShown();
  }
}

void AtomAutofillClient::PopupHidden() {
  if (delegate_) {
    delegate_->OnPopupHidden();
  }
}

void AtomAutofillClient::UpdateAutofillPopupDataListValues(
    const std::vector<base::string16>& values,
    const std::vector<base::string16>& labels) {
  if (api_web_contents_) {
    api_web_contents_->Emit("update-autofill-popup-data-list-values",
                            values, labels);
  }
}

void AtomAutofillClient::HideAutofillPopup() {
  if (api_web_contents_ && api_web_contents_->IsFocused()) {
    api_web_contents_->Emit("hide-autofill-popup");
  }
  // Password generation popups behave in the same fashion and should also
  // be hidden.
  BravePasswordManagerClient* password_client =
      BravePasswordManagerClient::FromWebContents(web_contents());
  if (password_client)
    password_client->HidePasswordGenerationPopup();
}

bool AtomAutofillClient::IsAutocompleteEnabled() {
  // For browser, Autocomplete is always enabled as part of Autofill.
  return GetPrefs()->GetBoolean(prefs::kAutofillEnabled);
}

void AtomAutofillClient::WebContentsDestroyed() {
  api_web_contents_ = nullptr;
}

void AtomAutofillClient::PropagateAutofillPredictions(
    content::RenderFrameHost* rfh,
    const std::vector<autofill::FormStructure*>& forms) {
  password_manager::ContentPasswordManagerDriver* driver =
      password_manager::ContentPasswordManagerDriver::GetForRenderFrameHost(
          rfh);
  if (driver) {
    driver->GetPasswordGenerationManager()->DetectFormsEligibleForGeneration(
        forms);
    driver->GetPasswordManager()->ProcessAutofillPredictions(driver, forms);
  }
}

void AtomAutofillClient::DidFillOrPreviewField(
    const base::string16& autofilled_value,
    const base::string16& profile_full_name) {
}

void AtomAutofillClient::DidInteractWithNonsecureCreditCardInput() {}

bool AtomAutofillClient::IsContextSecure() {
  content::SSLStatus ssl_status;
  content::NavigationEntry* navigation_entry =
      web_contents()->GetController().GetLastCommittedEntry();
  if (!navigation_entry)
     return false;

  ssl_status = navigation_entry->GetSSL();
  // Note: If changing the implementation below, also change
  // AwAutofillClient::IsContextSecure. See crbug.com/505388
  return navigation_entry->GetURL().SchemeIsCryptographic() &&
         ssl_status.certificate &&
         (!net::IsCertStatusError(ssl_status.cert_status) ||
          net::IsCertStatusMinorError(ssl_status.cert_status)) &&
         !(ssl_status.content_status &
           content::SSLStatus::RAN_INSECURE_CONTENT);
}

}  // namespace autofill
