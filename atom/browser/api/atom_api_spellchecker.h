// Copyright 2016 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_SPELLCHECKER_H_
#define ATOM_BROWSER_API_ATOM_API_SPELLCHECKER_H_

#include "atom/browser/api/trackable_object.h"
#include "brave/browser/brave_browser_context.h"
#include "native_mate/handle.h"

namespace atom {

namespace api {

class SpellChecker : public mate::TrackableObject<SpellChecker> {
 public:
  static mate::Handle<SpellChecker> Create(v8::Isolate* isolate,
                                  content::BrowserContext* browser_context);

  // mate::TrackableObject:
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  SpellChecker(v8::Isolate* isolate, content::BrowserContext* browser_context);
  ~SpellChecker() override;

  void AddWord(mate::Arguments* args);

  void RemoveWord(mate::Arguments* args);

 private:
  content::BrowserContext* browser_context_;  // not owned

  base::WeakPtrFactory<SpellChecker> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SpellChecker);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_SPELLCHECKER_H_
