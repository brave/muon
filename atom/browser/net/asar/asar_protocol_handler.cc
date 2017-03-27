// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/asar/asar_protocol_handler.h"

#include "atom/browser/net/asar/url_request_asar_job.h"
#include "net/base/filename_util.h"
#include "net/base/net_errors.h"

namespace asar {

AsarProtocolHandler::AsarProtocolHandler(
    const scoped_refptr<base::TaskRunner>& file_task_runner)
    : file_task_runner_(file_task_runner) {}

AsarProtocolHandler::~AsarProtocolHandler() {
}

net::URLRequestJob* AsarProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  return new URLRequestAsarJob(request, network_delegate, file_task_runner_);
}

bool AsarProtocolHandler::IsSafeRedirectTarget(const GURL& location) const {
  return false;
}

}  // namespace asar
