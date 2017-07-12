// Copyright 2016 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_spellchecker.h"

#include <string>

#include "atom/common/node_includes.h"
#include "chrome/browser/spellchecker/spellcheck_factory.h"
#include "chrome/browser/spellchecker/spellcheck_service.h"
#include "native_mate/dictionary.h"

namespace atom {

namespace api {

SpellChecker::SpellChecker(v8::Isolate* isolate,
                 content::BrowserContext* browser_context)
      : browser_context_(browser_context),
      weak_ptr_factory_(this) {
  Init(isolate);
}

SpellChecker::~SpellChecker() {}

void SpellChecker::AddWord(mate::Arguments* args) {
  if (args->Length() != 1) {
    args->ThrowError("Wrong number of arguments");
    return;
  }

  std::string word;
  if (!args->GetNext(&word)) {
    args->ThrowError("word is a required field");
    return;
  }

  if (browser_context_) {
    SpellcheckService* spellcheck =
      SpellcheckServiceFactory::GetForContext(browser_context_);
    if (spellcheck) {
      spellcheck->GetCustomDictionary()->AddWord(word);
    }
  }
}

void SpellChecker::RemoveWord(mate::Arguments* args) {
  if (args->Length() != 1) {
    args->ThrowError("Wrong number of arguments");
    return;
  }

  std::string word;
  if (!args->GetNext(&word)) {
    args->ThrowError("word is a required field");
    return;
  }

  if (browser_context_) {
    SpellcheckService* spellcheck =
      SpellcheckServiceFactory::GetForContext(browser_context_);
    if (spellcheck) {
      spellcheck->GetCustomDictionary()->RemoveWord(word);
    }
  }
}

// static
mate::Handle<SpellChecker> SpellChecker::Create(
    v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  return mate::CreateHandle(isolate,
                            new SpellChecker(isolate, browser_context));
}

// static
void SpellChecker::BuildPrototype(v8::Isolate* isolate,
                              v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "SpellChecker"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
    .SetMethod("addWord", &SpellChecker::AddWord)
    .SetMethod("removeWord", &SpellChecker::RemoveWord);
}

}  // namespace api

}  // namespace atom
