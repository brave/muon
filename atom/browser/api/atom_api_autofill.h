// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_AUTOFILL_H_
#define ATOM_BROWSER_API_ATOM_API_AUTOFILL_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "atom/browser/api/trackable_object.h"
#include "base/callback.h"
#include "base/single_thread_task_runner.h"
#include "brave/browser/brave_browser_context.h"
#include "components/autofill/core/browser/personal_data_manager_observer.h"
#include "components/password_manager/core/browser/password_store.h"
#include "native_mate/handle.h"

namespace autofill {
class AutofillProfile;
class PersonalDataManager;
}

namespace base {
class DictionaryValue;
class ListValue;
}

namespace brave {
class BraveBrowserContext;
}

using PasswordFormCallback =
  base::Callback<void(std::vector<std::unique_ptr<autofill::PasswordForm>>)>;

class BravePasswordStoreConsumer
  : public password_manager::PasswordStoreConsumer {
 public:
  explicit BravePasswordStoreConsumer(PasswordFormCallback callback)
    : callback_(callback) {}

  void OnGetPasswordStoreResults(
      std::vector<std::unique_ptr<autofill::PasswordForm>> results) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    callback_.Run(std::move(results));
  }

 private:
  PasswordFormCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(BravePasswordStoreConsumer);
};

namespace atom {

namespace api {

class Autofill : public mate::TrackableObject<Autofill>,
                 public autofill::PersonalDataManagerObserver,
                 public password_manager::PasswordStore::Observer {
 public:
  static mate::Handle<Autofill> Create(v8::Isolate* isolate,
                                  content::BrowserContext* browser_context);

  // mate::TrackableObject:
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  Autofill(v8::Isolate* isolate, content::BrowserContext* browser_context);
  ~Autofill() override;

  void AddProfile(const base::DictionaryValue& profile);
  autofill::AutofillProfile* GetProfile(const std::string& guid);
  void RemoveProfile(const std::string& guid);

  void AddCreditCard(const base::DictionaryValue& card);
  autofill::CreditCard* GetCreditCard(const std::string& guid);
  void RemoveCreditCard(const std::string& guid);

  void ClearAutocompleteData();
  void ClearAutofillData();

  void GetAutofillableLogins(mate::Arguments* args);
  void GetBlacklistLogins(mate::Arguments* args);

  void AddLogin(mate::Arguments* args);
  void UpdateLogin(mate::Arguments* args);
  void RemoveLogin(mate::Arguments* args);

  void ClearLogins();

  // PersonalDataManagerObserver
  void OnPersonalDataChanged() override;

  // password_manager::PasswordStoreConsumer
  void OnLoginsChanged(
    const password_manager::PasswordStoreChangeList& changes) override;

  Profile* profile();
  password_manager::PasswordStore* GetPasswordStore();
 private:
  void OnClearedAutocompleteData();
  void OnClearedAutofillData();

  content::BrowserContext* browser_context_;  // not owned

  autofill::PersonalDataManager* personal_data_manager_;  // not owned

  base::WeakPtrFactory<Autofill> weak_ptr_factory_;

  std::unique_ptr<BravePasswordStoreConsumer> password_list_consumer_;

  std::unique_ptr<BravePasswordStoreConsumer> password_blacked_list_consumer_;

  scoped_refptr<base::SingleThreadTaskRunner> db_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(Autofill);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_AUTOFILL_H_
