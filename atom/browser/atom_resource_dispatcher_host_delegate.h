// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
#define ATOM_BROWSER_ATOM_RESOURCE_DISPATCHER_HOST_DELEGATE_H_

#include <memory>
#include <vector>

#include "base/memory/ref_counted.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "content/public/browser/resource_dispatcher_host_delegate.h"

namespace safe_browsing {
class SafeBrowsingService;
}

namespace atom {

class AtomResourceDispatcherHostDelegate
    : public content::ResourceDispatcherHostDelegate {
 public:
  AtomResourceDispatcherHostDelegate();

  void RequestBeginning(net::URLRequest* request,
                        content::ResourceContext* resource_context,
                        content::AppCacheService* appcache_service,
                        content::ResourceType resource_type,
                        std::vector<std::unique_ptr<content::ResourceThrottle>>*
                            throttles) override;

 protected:
    virtual void AppendStandardResourceThrottles(
      net::URLRequest* request,
      content::ResourceContext* resource_context,
      content::ResourceType resource_type,
      std::vector<std::unique_ptr<content::ResourceThrottle>>* throttles);

 private:
    scoped_refptr<safe_browsing::SafeBrowsingService> safe_browsing_;
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
