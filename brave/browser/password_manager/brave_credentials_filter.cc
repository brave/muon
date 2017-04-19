// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/password_manager/brave_credentials_filter.h"

namespace password_manager {

BraveCredentialsFilter::BraveCredentialsFilter() = default;

BraveCredentialsFilter::~BraveCredentialsFilter() = default;

std::vector<std::unique_ptr<autofill::PasswordForm>>
BraveCredentialsFilter::FilterResults(
    std::vector<std::unique_ptr<autofill::PasswordForm>> results) const {
  FilterResultsPtr(&results);
  return results;
}

bool BraveCredentialsFilter::ShouldSave(
    const autofill::PasswordForm& form) const {
  return true;
}

void BraveCredentialsFilter::ReportFormLoginSuccess(
    const PasswordFormManager& form_manager) const {}

void BraveCredentialsFilter::FilterResultsPtr(
    std::vector<std::unique_ptr<autofill::PasswordForm>>* results) const {}

}  // namespace password_manager
