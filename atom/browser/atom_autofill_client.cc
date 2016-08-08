// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_autofill_client.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "chrome/browser/autofill/personal_data_manager_factory.h"
#include "chrome/browser/autofill/risk_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/autofill/autofill_popup_controller_impl.h"
#include "chrome/browser/ui/autofill/create_card_unmask_prompt_view.h"
#include "chrome/browser/ui/autofill/credit_card_scanner_controller.h"
#include "chrome/browser/ui/autofill/save_card_bubble_controller_impl.h"
#include "chrome/browser/web_data_service_factory.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/content/common/autofill_messages.h"
#include "components/autofill/core/browser/ui/card_unmask_prompt_view.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "ui/gfx/geometry/rect.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(autofill::AtomAutofillClient);

namespace autofill {

AtomAutofillClient::AtomAutofillClient(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      unmask_controller_(
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext()),
          Profile::FromBrowserContext(web_contents->GetBrowserContext())
              ->IsOffTheRecord()) {
  DCHECK(web_contents);

  // Since ZoomController is also a WebContentsObserver, we need to be careful
  // about disconnecting from it since the relative order of destruction of
  // WebContentsObservers is not guaranteed. ZoomController silently clears
  // its ZoomObserver list during WebContentsDestroyed() so there's no need
  // to explicitly remove ourselves on destruction.
  ui_zoom::ZoomController* zoom_controller =
      ui_zoom::ZoomController::FromWebContents(web_contents);
  // There may not always be a ZoomController, e.g. in tests.
  if (zoom_controller)
    zoom_controller->AddObserver(this);
}

AtomAutofillClient::~AtomAutofillClient() {
  // NOTE: It is too late to clean up the autofill popup; that cleanup process
  // requires that the WebContents instance still be valid and it is not at
  // this point (in particular, the WebContentsImpl destructor has already
  // finished running and we are now in the base class destructor).
  DCHECK(!popup_controller_);
}

PersonalDataManager* AtomAutofillClient::GetPersonalDataManager() {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  return PersonalDataManagerFactory::GetForProfile(
      profile->GetOriginalProfile());
}

scoped_refptr<AutofillWebDataService> AtomAutofillClient::GetDatabase() {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  return WebDataServiceFactory::GetAutofillWebDataForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);
}

PrefService* AtomAutofillClient::GetPrefs() {
  return Profile::FromBrowserContext(web_contents()->GetBrowserContext())
      ->GetPrefs();
}

sync_driver::SyncService* AtomAutofillClient::GetSyncService() {
  return nullptr;
}

IdentityProvider* AtomAutofillClient::GetIdentityProvider() {
  return nullptr;
}

rappor::RapporService* AtomAutofillClient::GetRapporService() {
  return g_browser_process->rappor_service();
}

void AtomAutofillClient::ShowAutofillSettings() {
}

void AtomAutofillClient::ShowUnmaskPrompt(
    const CreditCard& card,
    UnmaskCardReason reason,
    base::WeakPtr<CardUnmaskDelegate> delegate) {
  unmask_controller_.ShowPrompt(
      CreateCardUnmaskPromptView(&unmask_controller_, web_contents()),
      card, reason, delegate);
}

void AtomAutofillClient::OnUnmaskVerificationResult(
    PaymentsRpcResult result) {
  unmask_controller_.OnVerificationResult(result);
}

void AtomAutofillClient::ConfirmSaveCreditCardLocally(
    const CreditCard& card,
    const base::Closure& callback) {
  // Do lazy initialization of SaveCardBubbleControllerImpl.
  autofill::SaveCardBubbleControllerImpl::CreateForWebContents(
      web_contents());
  autofill::SaveCardBubbleControllerImpl* controller =
      autofill::SaveCardBubbleControllerImpl::FromWebContents(web_contents());
  controller->ShowBubbleForLocalSave(card, callback);
#endif
}

void AtomAutofillClient::ConfirmSaveCreditCardToCloud(
    const CreditCard& card,
    std::unique_ptr<base::DictionaryValue> legal_message,
    const base::Closure& callback) {
  // Do lazy initialization of SaveCardBubbleControllerImpl.
  autofill::SaveCardBubbleControllerImpl::CreateForWebContents(web_contents());
  autofill::SaveCardBubbleControllerImpl* controller =
      autofill::SaveCardBubbleControllerImpl::FromWebContents(web_contents());
  controller->ShowBubbleForUpload(card, std::move(legal_message), callback);
#endif
}

void AtomAutofillClient::LoadRiskData(
    const base::Callback<void(const std::string&)>& callback) {
  ::autofill::LoadRiskData(0, web_contents(), callback);
}

bool AtomAutofillClient::HasCreditCardScanFeature() {
  return CreditCardScannerController::HasCreditCardScanFeature();
}

void AtomAutofillClient::ScanCreditCard(
    const CreditCardScanCallback& callback) {
  CreditCardScannerController::ScanCreditCard(web_contents(), callback);
}

void AtomAutofillClient::ShowAutofillPopup(
    const gfx::RectF& element_bounds,
    base::i18n::TextDirection text_direction,
    const std::vector<autofill::Suggestion>& suggestions,
    base::WeakPtr<AutofillPopupDelegate> delegate) {
  // Convert element_bounds to be in screen space.
  gfx::Rect client_area = web_contents()->GetContainerBounds();
  gfx::RectF element_bounds_in_screen_space =
      element_bounds + client_area.OffsetFromOrigin();

  // Will delete or reuse the old |popup_controller_|.
  popup_controller_ =
      AutofillPopupControllerImpl::GetOrCreate(popup_controller_,
                                               delegate,
                                               web_contents(),
                                               web_contents()->GetNativeView(),
                                               element_bounds_in_screen_space,
                                               text_direction);

  popup_controller_->Show(suggestions);
}

void AtomAutofillClient::UpdateAutofillPopupDataListValues(
    const std::vector<base::string16>& values,
    const std::vector<base::string16>& labels) {
  if (popup_controller_.get())
    popup_controller_->UpdateDataListValues(values, labels);
}

void AtomAutofillClient::HideAutofillPopup() {
  if (popup_controller_.get())
    popup_controller_->Hide();
}

bool AtomAutofillClient::IsAutocompleteEnabled() {
  // For browser, Autocomplete is always enabled as part of Autofill.
  return GetPrefs()->GetBoolean(prefs::kAutofillEnabled);
}

void AtomAutofillClient::MainFrameWasResized(bool width_changed) {
  HideAutofillPopup();
}

void AtomAutofillClient::WebContentsDestroyed() {
  HideAutofillPopup();
}

void AtomAutofillClient::OnZoomChanged(
    const ui_zoom::ZoomController::ZoomChangedEventData& data) {
  HideAutofillPopup();
}

void AtomAutofillClient::PropagateAutofillPredictions(
    content::RenderFrameHost* rfh,
    const std::vector<autofill::FormStructure*>& forms) {
}

void AtomAutofillClient::DidFillOrPreviewField(
    const base::string16& autofilled_value,
    const base::string16& profile_full_name) {
}

void AtomAutofillClient::OnFirstUserGestureObserved() {
  ContentAutofillDriverFactory* factory =
      ContentAutofillDriverFactory::FromWebContents(web_contents());
  DCHECK(factory);

  for (content::RenderFrameHost* frame : web_contents()->GetAllFrames()) {
    // No need to notify non-live frames.
    // And actually they have no corresponding drivers in the factory's map.
    if (!frame->IsRenderFrameLive())
      continue;
    ContentAutofillDriver* driver = factory->DriverForFrame(frame);
    DCHECK(driver);
    driver->NotifyFirstUserGestureObservedInTab();
  }
}

bool AtomAutofillClient::IsContextSecure(const GURL& form_origin) {
  content::SSLStatus ssl_status;
  content::NavigationEntry* navigation_entry =
      web_contents()->GetController().GetLastCommittedEntry();
  if (!navigation_entry)
     return false;

  ssl_status = navigation_entry->GetSSL();
  // Note: If changing the implementation below, also change
  // AwAutofillClient::IsContextSecure. See crbug.com/505388
  return ssl_status.security_style == content::SECURITY_STYLE_AUTHENTICATED &&
         !(ssl_status.content_status &
           content::SSLStatus::RAN_INSECURE_CONTENT);
}

}  // namespace autofill
