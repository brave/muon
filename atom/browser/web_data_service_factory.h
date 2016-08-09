// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_WEB_DATA_SERVICE_FACTORY_H_
#define ATOM_BROWSER_WEB_DATA_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/service_access_type.h"

namespace base {
template <typename T> struct DefaultSingletonTraits;
}

class Profile;
class WebDataServiceWrapper;

namespace autofill {
class AutofillWebDataService;
}

// Singleton that owns all WebDataServiceWrappers and associates them with
// Profiles.
class WebDataServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the WebDataServiceWrapper associated with the |profile|.
  static WebDataServiceWrapper* GetForProfile(Profile* profile,
                                              ServiceAccessType access_type);

  // Returns the AutofillWebDataService associated with the |profile|.
  static scoped_refptr<autofill::AutofillWebDataService>
  GetAutofillWebDataForProfile(Profile* profile, ServiceAccessType access_type);

  static WebDataServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<WebDataServiceFactory>;

  WebDataServiceFactory();
  ~WebDataServiceFactory() override;

  // |BrowserContextKeyedBaseFactory| methods:
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(WebDataServiceFactory);
};

#endif  // ATOM_BROWSER_WEB_DATA_SERVICE_FACTORY_H_
