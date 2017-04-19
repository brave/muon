// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_PASSWORD_MANAGER_BRAVE_CREDENTIALS_FILTER_H_
#define BRAVE_BROWSER_PASSWORD_MANAGER_BRAVE_CREDENTIALS_FILTER_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "components/password_manager/core/browser/credentials_filter.h"

namespace password_manager {

class BraveCredentialsFilter : public CredentialsFilter {
 public:
  BraveCredentialsFilter();

  ~BraveCredentialsFilter() override;

  // CredentialsFilter
  std::vector<std::unique_ptr<autofill::PasswordForm>> FilterResults(
      std::vector<std::unique_ptr<autofill::PasswordForm>> results)
      const override;
  bool ShouldSave(const autofill::PasswordForm& form) const override;
  void ReportFormLoginSuccess(
      const PasswordFormManager& form_manager) const override;

  virtual void FilterResultsPtr(
      std::vector<std::unique_ptr<autofill::PasswordForm>>* results) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveCredentialsFilter);
};

}  // namespace password_manager

#endif  // BRAVE_BROWSER_PASSWORD_MANAGER_BRAVE_CREDENTIALS_FILTER_H_
