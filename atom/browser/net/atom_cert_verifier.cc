// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_cert_verifier.h"

#include <utility>

#include "atom/browser/browser.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"
#include "net/cert/crl_set.h"
#include "net/cert/x509_certificate.h"

using content::BrowserThread;

namespace atom {

namespace {

void OnResult(
    net::CertVerifyResult* verify_result,
    net::CompletionOnceCallback callback,
    bool result) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(std::move(callback), result ? net::OK : net::ERR_FAILED));
}

}  // namespace

AtomCertVerifier::AtomCertVerifier()
    : default_cert_verifier_(net::CertVerifier::CreateDefault()) {
}

AtomCertVerifier::~AtomCertVerifier() {
}

void AtomCertVerifier::SetVerifyProc(VerifyProc proc) {
  verify_proc_ = std::move(proc);
}

int AtomCertVerifier::Verify(
    const RequestParams& params,
    net::CRLSet* crl_set,
    net::CertVerifyResult* verify_result,
    net::CompletionOnceCallback callback,
    std::unique_ptr<Request>* out_req,
    const net::NetLogWithSource& net_log) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!verify_proc_)
    return default_cert_verifier_->Verify(
        params, crl_set, verify_result, std::move(callback), out_req, net_log);

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          std::move(verify_proc_), params.hostname(), params.certificate(),
          base::BindOnce(OnResult, verify_result, std::move(callback))));
  return net::ERR_IO_PENDING;
}

}  // namespace atom
