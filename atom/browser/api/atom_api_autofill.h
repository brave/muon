// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_AUTOFILL_H_
#define ATOM_BROWSER_API_ATOM_API_AUTOFILL_H_

#include <string>

#include "atom/browser/api/trackable_object.h"
#include "base/callback.h"
#include "brave/browser/brave_browser_context.h"
#include "components/autofill/core/browser/personal_data_manager_observer.h"
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

namespace atom {

namespace api {

class Autofill : public mate::TrackableObject<Autofill>,
                 public autofill::PersonalDataManagerObserver {
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

  // PersonalDataManagerObserver
  void OnPersonalDataChanged() override;

  Profile* profile();
 private:
  void OnClearedAutocompleteData();
  void OnClearedAutofillData();

  content::BrowserContext* browser_context_;  // not owned

  autofill::PersonalDataManager* personal_data_manager_;  // not owned

  base::WeakPtrFactory<Autofill> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(Autofill);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_AUTOFILL_H_
