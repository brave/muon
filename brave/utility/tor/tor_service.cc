// Copyright (c) 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/utility/tor/tor_service.h"

#include <utility>

#include "build/build_config.h"
#include "brave/utility/tor/tor_launcher_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace {

void OnTorLauncherRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    tor::mojom::TorLauncherRequest request) {
  mojo::MakeStrongBinding(
      base::MakeUnique<brave::TorLauncherImpl>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace

TorService::TorService() {}

TorService::~TorService() {}

std::unique_ptr<service_manager::Service>
TorService::CreateService() {
  return base::MakeUnique<TorService>();
}

void TorService::OnStart() {
  ref_factory_.reset(new service_manager::ServiceContextRefFactory(
      context()->CreateQuitClosure()));
  registry_.AddInterface(
      base::Bind(&OnTorLauncherRequest, ref_factory_.get()));
}

void TorService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}
