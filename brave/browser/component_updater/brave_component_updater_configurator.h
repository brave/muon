// Copyright 2016 Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_UPDATER_CONFIGURATOR_H_
#define BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_UPDATER_CONFIGURATOR_H_

#include "base/memory/ref_counted.h"
#include "components/update_client/configurator.h"

namespace base {
class CommandLine;
}

namespace net {
class URLRequestContextGetter;
}

namespace component_updater {

scoped_refptr<update_client::Configurator>
MakeBraveComponentUpdaterConfigurator(
    const base::CommandLine* cmdline,
    net::URLRequestContextGetter* context_getter);

}  // namespace component_updater

#endif  // BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_UPDATER_CONFIGURATOR_H_
