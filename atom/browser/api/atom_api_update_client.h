// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_UPDATE_CLIENT_H_
#define ATOM_BROWSER_API_ATOM_API_UPDATE_CLIENT_H_

#include <string>

#include "atom/browser/api/trackable_object.h"
#include "base/callback.h"
#include "brave/browser/brave_browser_context.h"
#include "native_mate/handle.h"

namespace brave {
class BraveBrowserContext;
}

namespace atom {

namespace api {

class UpdateClient : public mate::TrackableObject<UpdateClient> {
 public:
  static mate::Handle<UpdateClient> Create(v8::Isolate* isolate,
                                  content::BrowserContext* browser_context);

  // mate::TrackableObject:
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  UpdateClient(v8::Isolate* isolate, content::BrowserContext* browser_context);
  ~UpdateClient() override;
  void Install(const std::string& extension_id);
  void Update(const std::string& extension_id);
  brave::BraveBrowserContext* browser_context() {
    return static_cast<brave::BraveBrowserContext*>(browser_context_);
  }

 private:
  content::BrowserContext* browser_context_;  // not owned

  DISALLOW_COPY_AND_ASSIGN(UpdateClient);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_UPDATE_CLIENT_H_
