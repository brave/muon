// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_PASSWORD_MANAGER_BRAVE_PASSWORD_MANAGER_CLIENT_H_
#define BRAVE_BROWSER_PASSWORD_MANAGER_BRAVE_PASSWORD_MANAGER_CLIENT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/optional.h"
#include "brave/browser/password_manager/brave_credentials_filter.h"
#include "components/autofill/content/common/autofill_driver.mojom.h"
#include "components/password_manager/content/browser/content_credential_manager.h"
#include "components/password_manager/content/browser/content_password_manager_driver_factory.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_manager_metrics_recorder.h"
#include "components/password_manager/core/browser/password_reuse_detection_manager.h"
#include "components/prefs/pref_member.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents_binding_set.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/gfx/geometry/rect.h"

class Profile;

namespace atom {
namespace api {
class WebContents;
}
}

namespace autofill {
class PasswordGenerationPopupObserver;
class PasswordGenerationPopupControllerImpl;
}

namespace content {
class WebContents;
}

// BravePasswordManagerClient implements the PasswordManagerClient interface.
class BravePasswordManagerClient
    :  public password_manager::PasswordManagerClient,
      public content::WebContentsObserver,
      public content::WebContentsUserData<BravePasswordManagerClient>,
      public autofill::mojom::PasswordManagerClient,
      public content::RenderWidgetHost::InputEventObserver {
 public:
  BravePasswordManagerClient(content::WebContents* web_contents,
                             autofill::AutofillClient* autofill_client);
  ~BravePasswordManagerClient() override;

  void Initialize(atom::api::WebContents* api_web_contents);

  void DidClickSave();

  void DidClickNever();

  void DidClickUpdate();

  void DidClickNoUpdate();

  // PasswordManagerClient implementation.
  bool IsSavingAndFillingEnabledForCurrentPage() const override;
  bool IsFillingEnabledForCurrentPage() const override;
  void PostHSTSQueryForHost(const GURL& origin,
                            const HSTSCallback& callback) const override;
  bool OnCredentialManagerUsed() override;
  bool PromptUserToSaveOrUpdatePassword(
      std::unique_ptr<password_manager::PasswordFormManager> form_to_save,
      bool update_password) override;
  void ShowManualFallbackForSaving(
      std::unique_ptr<password_manager::PasswordFormManager> form_to_save,
      bool has_generated_password,
      bool is_update) override;
  void HideManualFallbackForSaving() override;
  bool PromptUserToChooseCredentials(
      std::vector<std::unique_ptr<autofill::PasswordForm>> local_forms,
      const GURL& origin,
      const CredentialsCallback& callback) override;
  void ForceSavePassword() override;
  void GeneratePassword() override;
  void NotifyUserAutoSignin(
      std::vector<std::unique_ptr<autofill::PasswordForm>> local_forms,
      const GURL& origin) override;
  void NotifyUserCouldBeAutoSignedIn(
      std::unique_ptr<autofill::PasswordForm> form) override;
  void NotifySuccessfulLoginWithExistingPassword(
      const autofill::PasswordForm& form) override;
  void NotifyStorePasswordCalled() override;
  void AutomaticPasswordSave(
      std::unique_ptr<password_manager::PasswordFormManager> saved_form_manager)
      override;
  void PasswordWasAutofilled(
      const std::map<base::string16, const autofill::PasswordForm*>&
          best_matches,
      const GURL& origin,
      const std::vector<const autofill::PasswordForm*>* federated_matches)
      const override;
  PrefService* GetPrefs() override;
  password_manager::PasswordStore* GetPasswordStore() const override;
#if defined(SAFE_BROWSING_DB_LOCAL)
  safe_browsing::PasswordProtectionService* GetPasswordProtectionService()
      const override;
  void CheckSafeBrowsingReputation(const GURL& form_action,
                                   const GURL& frame_url) override;
  void CheckProtectedPasswordEntry(
      const std::string& password_saved_domain,
      bool password_field_exists) override;

#endif
  password_manager::PasswordSyncState GetPasswordSyncState() const override;
  bool WasLastNavigationHTTPError() const override;
  net::CertStatus GetMainFrameCertStatus() const override;
  bool IsIncognito() const override;
  const password_manager::PasswordManager* GetPasswordManager() const override;
  autofill::AutofillManager* GetAutofillManagerForMainFrame() override;
  const GURL& GetMainFrameURL() const override;
  bool IsMainFrameSecure() const override;
  const GURL& GetLastCommittedEntryURL() const override;
  void AnnotateNavigationEntry(bool has_password_field) override;
  const password_manager::CredentialsFilter* GetStoreResultFilter()
      const override;
  const password_manager::LogManager* GetLogManager() const override;

  // autofill::mojom::PasswordManagerClient overrides.
  void ShowPasswordGenerationPopup(const gfx::RectF& bounds,
                                   int max_length,
                                   const base::string16& generation_element,
                                   bool is_manually_triggered,
                                   const autofill::PasswordForm& form) override;
  void ShowPasswordEditingPopup(const gfx::RectF& bounds,
                                const autofill::PasswordForm& form) override;
  void GenerationAvailableForForm(const autofill::PasswordForm& form) override;
  void HidePasswordGenerationPopup() override;

  password_manager::PasswordManagerMetricsRecorder& GetMetricsRecorder()
      override;

  ukm::SourceId GetUkmSourceId() override;

  static void CreateForWebContentsWithAutofillClient(
      content::WebContents* contents,
      autofill::AutofillClient* autofill_client);

  static bool IsPossibleConfirmPasswordForm(const autofill::PasswordForm& form);

  // Observer for PasswordGenerationPopup events. Used for testing.
  void SetTestObserver(autofill::PasswordGenerationPopupObserver* observer);

  static void BindCredentialManager(
      password_manager::mojom::CredentialManagerRequest request,
      content::RenderFrameHost* render_frame_host);

  // A helper method to determine whether a save/update bubble can be shown
  // on this |url|.
  static bool CanShowBubbleOnURL(const GURL& url);

 private:
  friend class content::WebContentsUserData<BravePasswordManagerClient>;

  // content::WebContentsObserver overrides.
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  // content::RenderWidgetHost::InputEventObserver overrides.
  void OnInputEvent(const blink::WebInputEvent&) override;

  // Given |bounds| in the renderers coordinate system, return the same bounds
  // in the screens coordinate system.
  gfx::RectF GetBoundsInScreenSpace(const gfx::RectF& bounds);

  // Checks if the current page fulfils the conditions for the password manager
  // to be active on it, for example Sync credentials are not saved or auto
  // filled.
  bool IsPasswordManagementEnabledForCurrentPage() const;

  // Shows the dialog where the user can accept or decline the global autosignin
  // setting as a first run experience. The dialog won't appear in Incognito or
  // when the autosign-in is off.
  void PromptUserToEnableAutosigninIfNecessary();

  // Called as a response to PromptUserToChooseCredentials. nullptr in |form|
  // means that nothing was chosen. |one_local_credential| is true if there was
  // just one local credential to be chosen from.
  void OnCredentialsChosen(const CredentialsCallback& callback,
                           bool one_local_credential,
                           const autofill::PasswordForm* form);

  // Returns true if this profile has metrics reporting and active sync
  // without custom sync passphrase.
  static bool ShouldAnnotateNavigationEntries(Profile* profile);

  Profile* const profile_;

  password_manager::PasswordManager password_manager_;

  password_manager::PasswordReuseDetectionManager
      password_reuse_detection_manager_;

  password_manager::ContentPasswordManagerDriverFactory* driver_factory_;

  // As a mojo service, will be registered into service registry
  // of the main frame host by ChromeContentBrowserClient
  // once main frame host was created.
  password_manager::ContentCredentialManager credential_manager_;

  content::WebContentsFrameBindingSet<autofill::mojom::PasswordManagerClient>
      password_manager_client_bindings_;

  // Observer for password generation popup.
  autofill::PasswordGenerationPopupObserver* observer_;

  // Set to false to disable password saving (will no longer ask if you
  // want to save passwords and also won't fill the passwords).
  BooleanPrefMember saving_and_filling_passwords_enabled_;

  const password_manager::BraveCredentialsFilter credentials_filter_;

  std::unique_ptr<password_manager::LogManager> log_manager_;

  // Set during 'NotifyUserCouldBeAutoSignedIn' in order to store the
  // form for potential use during 'NotifySuccessfulLoginWithExistingPassword'.
  std::unique_ptr<autofill::PasswordForm> possible_auto_sign_in_;

  std::unique_ptr<password_manager::PasswordFormManager> form_to_save_;

  atom::api::WebContents* api_web_contents_;

  // Recorder of metrics that is associated with the last committed navigation
  // of the WebContents owning this BravePasswordManagerClient. May be unset at
  // times. Sends statistics on destruction.
  base::Optional<password_manager::PasswordManagerMetricsRecorder>
      metrics_recorder_;

  DISALLOW_COPY_AND_ASSIGN(BravePasswordManagerClient);
};

#endif  // BRAVE_BROWSER_PASSWORD_MANAGER_BRAVE_PASSWORD_MANAGER_CLIENT_H_
