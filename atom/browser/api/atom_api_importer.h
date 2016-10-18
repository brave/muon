// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_IMPORTER_H_
#define ATOM_BROWSER_API_ATOM_API_IMPORTER_H_

#include <map>
#include <string>

#include "atom/browser/api/event_emitter.h"
#include "atom/browser/api/trackable_object.h"
#include "base/callback.h"
#include "base/values.h"
#include "chrome/browser/importer/importer_progress_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/importer/importer_data_types.h"
#include "native_mate/handle.h"

class ExternalProcessImporterHost;
class ImporterList;
class ProfileWriter;

namespace atom {

namespace api {

class Importer: public mate::EventEmitter<Importer>,
                public importer::ImporterProgressObserver {
 public:
  static mate::Handle<Importer> Create(v8::Isolate* isolate);

  // mate::TrackableObject:
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  explicit Importer(v8::Isolate* isolate);
  ~Importer() override;

 private:
  void InitializeImporter();
  void InitializePage();

  void ImportData(const base::DictionaryValue& data);
  void ImportHTML(const base::FilePath& path);
  void StartImport(const importer::SourceProfile& source_profile,
                   uint16_t imported_items);

  // importer::ImporterProgressObserver:
  void ImportStarted() override;
  void ImportItemStarted(importer::ImportItem item) override;
  void ImportItemEnded(importer::ImportItem item) override;
  void ImportEnded() override;

  // If non-null it means importing is in progress. ImporterHost takes care
  // of deleting itself when import is complete.
  ExternalProcessImporterHost* importer_host_;  // weak

  std::unique_ptr<ImporterList> importer_list_;

  scoped_refptr<ProfileWriter> profile_writer_;

  bool import_did_succeed_;

  DISALLOW_COPY_AND_ASSIGN(Importer);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_IMPORTER_H_
