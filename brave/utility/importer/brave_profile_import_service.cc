// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/utility/importer/brave_profile_import_service.h"

#include <string>
#include <utility>

#include "brave/utility/importer/brave_profile_import_impl.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

#if defined(OS_MACOSX)
#include <stdlib.h>

#include "chrome/common/importer/firefox_importer_utils.h"
#endif

namespace {

void OnProfileImportRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    chrome::mojom::ProfileImportRequest request) {
  mojo::MakeStrongBinding(
      base::MakeUnique<BraveProfileImportImpl>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace

BraveProfileImportService::BraveProfileImportService()
    : ProfileImportService() {}

BraveProfileImportService::~BraveProfileImportService() {}

std::unique_ptr<service_manager::Service>
BraveProfileImportService::CreateService() {
  return base::MakeUnique<BraveProfileImportService>();
}

void BraveProfileImportService::OnStart() {
  ref_factory_.reset(new service_manager::ServiceContextRefFactory(
      context()->CreateQuitClosure()));
  registry_.AddInterface(
      base::Bind(&OnProfileImportRequest, ref_factory_.get()));

#if defined(OS_MACOSX)
  std::string dylib_path = GetFirefoxDylibPath().value();
  if (!dylib_path.empty())
    ::setenv("DYLD_FALLBACK_LIBRARY_PATH", dylib_path.c_str(),
             1 /* overwrite */);
#endif
}
