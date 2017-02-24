// Copyright 2016 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "brave/browser/brave_browser_context.h"

#include "atom/browser/net/atom_url_request_job_factory.h"
#include "base/path_service.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "brave/browser/brave_permission_manager.h"
#include "brightray/browser/brightray_paths.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "common/application_info.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/autofill/core/browser/webdata/autofill_table.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/guest_view/browser/guest_view_manager.h"
#include "components/guest_view/browser/guest_view_manager_delegate.h"
#include "components/guest_view/common/guest_view_constants.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_filter.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "components/sync_preferences/pref_service_syncable_factory.h"
#include "components/user_prefs/user_prefs.h"
#include "components/zoom/zoom_event_manager.h"
#include "components/webdata/common/webdata_constants.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/dom_storage_context.h"
#include "content/public/browser/storage_partition.h"
#include "extensions/features/features.h"
#include "net/base/escape.h"
#include "net/cookies/cookie_store.h"
#include "net/url_request/url_request_context.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "atom/browser/extensions/atom_browser_client_extensions_part.h"
#include "atom/browser/extensions/atom_extensions_network_delegate.h"
#include "atom/browser/extensions/atom_extension_system_factory.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/api/web_request/web_request_api.h"
#include "extensions/browser/extension_pref_store.h"
#include "extensions/browser/extension_pref_value_map_factory.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_protocols.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extensions_browser_client.h"
#endif

#if BUILDFLAG(ENABLE_PLUGINS)
#include "brave/browser/plugins/brave_plugin_service_filter.h"
#include "chrome/browser/pepper_flash_settings_manager.h"
#endif

using content::BrowserThread;
using content::HostZoomMap;

#if BUILDFLAG(ENABLE_EXTENSIONS)
namespace {

void NotifyOTRProfileCreatedOnIOThread(void* original_profile,
                                       void* otr_profile) {
  extensions::ExtensionWebRequestEventRouter::GetInstance()
      ->OnOTRBrowserContextCreated(original_profile, otr_profile);
}

void NotifyOTRProfileDestroyedOnIOThread(void* original_profile,
                                         void* otr_profile) {
  extensions::ExtensionWebRequestEventRouter::GetInstance()
      ->OnOTRBrowserContextDestroyed(original_profile, otr_profile);
}

}  // namespace
#endif

namespace brave {

const char kPersistPrefix[] = "persist:";
const int kPersistPrefixLength = 8;

void DatabaseErrorCallback(sql::InitStatus init_status,
                           const std::string& diagnostics) {
  LOG(WARNING) << "initializing autocomplete database failed";
}

BraveBrowserContext::BraveBrowserContext(const std::string& partition,
                           bool in_memory,
                           const base::DictionaryValue& options,
                           scoped_refptr<base::SequencedTaskRunner> task_runner)
    : Profile(partition, in_memory, options),
      pref_registry_(new user_prefs::PrefRegistrySyncable),
      has_parent_(false),
      original_context_(nullptr),
      partition_(partition),
      ready_(
        new base::WaitableEvent(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED)) {
  std::string parent_partition;
  if (options.GetString("parent_partition", &parent_partition)) {
    has_parent_ = true;
    original_context_ = static_cast<BraveBrowserContext*>(
        atom::AtomBrowserContext::From(parent_partition, false).get());
  }

  if (in_memory) {
    original_context_ = static_cast<BraveBrowserContext*>(
        atom::AtomBrowserContext::From(partition, false).get());
    original_context_->otr_context_ = this;
  }
  CreateProfilePrefs(task_runner);
  if (original_context_) {
    TrackZoomLevelsFromParent();
  }
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (IsOffTheRecord()) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&NotifyOTRProfileCreatedOnIOThread,
          base::Unretained(original_context_.get()),
          base::Unretained(this)));
  }
#endif
}

BraveBrowserContext::~BraveBrowserContext() {
  MaybeSendDestroyedNotification();

  if (track_zoom_subscription_.get())
    track_zoom_subscription_.reset(nullptr);

  if (parent_default_zoom_level_subscription_.get())
    parent_default_zoom_level_subscription_.reset(nullptr);

  if (user_prefs_registrar_.get())
    user_prefs_registrar_->RemoveAll();

  #if BUILDFLAG(ENABLE_PLUGINS)
    BravePluginServiceFilter::GetInstance()->UnregisterResourceContext(
        GetResourceContext());
  #endif

  if (IsOffTheRecord()) {
    auto user_prefs = user_prefs::UserPrefs::Get(this);
    if (user_prefs)
      user_prefs->ClearMutableValues();
#if BUILDFLAG(ENABLE_EXTENSIONS)
    ExtensionPrefValueMapFactory::GetForBrowserContext(
        original_context_.get())->ClearAllIncognitoSessionOnlyPreferences();
#endif
  }

  if (!IsOffTheRecord() && !HasParentContext()) {
    autofill_data_->ShutdownOnUIThread();
    web_database_->ShutdownDatabase();

    bool prefs_loaded = user_prefs_->GetInitializationStatus() !=
        PrefService::INITIALIZATION_STATUS_WAITING;

    if (prefs_loaded) {
      user_prefs_->CommitPendingWrite();
    }
  }

  BrowserContextDependencyManager::GetInstance()->
      DestroyBrowserContextServices(this);

  if (IsOffTheRecord()) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&NotifyOTRProfileDestroyedOnIOThread,
          base::Unretained(original_context_.get()), base::Unretained(this)));
#endif
  }

  ShutdownStoragePartitions();
}

// static
BraveBrowserContext* BraveBrowserContext::FromBrowserContext(
    content::BrowserContext* browser_context) {
  return static_cast<BraveBrowserContext*>(browser_context);
}

Profile* BraveBrowserContext::GetOffTheRecordProfile() {
  return otr_context();
}

bool BraveBrowserContext::HasOffTheRecordProfile() {
  return !!otr_context();
}

Profile* BraveBrowserContext::GetOriginalProfile() {
  return original_context();
}

bool BraveBrowserContext::IsSameProfile(Profile* profile) {
  return GetOriginalProfile() == profile->GetOriginalProfile();
}

bool BraveBrowserContext::HasParentContext() {
  return has_parent_;
}

BraveBrowserContext* BraveBrowserContext::original_context() {
  if (original_context_) {
    return original_context_.get();
  }
  return this;
}

BraveBrowserContext* BraveBrowserContext::otr_context() {
  if (IsOffTheRecord()) {
    return this;
  }

  if (HasParentContext())
    return original_context_->otr_context();

  if (otr_context_) {
    return otr_context_;
  }

  return nullptr;
}

content::BrowserPluginGuestManager* BraveBrowserContext::GetGuestManager() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (!guest_view::GuestViewManager::FromBrowserContext(this)) {
    guest_view::GuestViewManager::CreateWithDelegate(
        this,
        extensions::ExtensionsAPIClient::Get()->CreateGuestViewManagerDelegate(
            this));
  }
#endif
  return guest_view::GuestViewManager::FromBrowserContext(this);
}

void BraveBrowserContext::TrackZoomLevelsFromParent() {
  // Here we only want to use zoom levels stored in the main-context's default
  // storage partition. We're not interested in zoom levels in special
  // partitions, e.g. those used by WebViewGuests.
  HostZoomMap* host_zoom_map = HostZoomMap::GetDefaultForBrowserContext(this);
  HostZoomMap* parent_host_zoom_map =
      HostZoomMap::GetDefaultForBrowserContext(original_context_.get());
  host_zoom_map->CopyFrom(parent_host_zoom_map);
  // Observe parent profile's HostZoomMap changes so they can also be applied
  // to this profile's HostZoomMap.
  track_zoom_subscription_ = parent_host_zoom_map->AddZoomLevelChangedCallback(
      base::Bind(&BraveBrowserContext::OnParentZoomLevelChanged,
                 base::Unretained(this)));
  if (!original_context_->GetZoomLevelPrefs())
    return;

  // Also track changes to the parent profile's default zoom level.
  parent_default_zoom_level_subscription_ =
      original_context_->GetZoomLevelPrefs()->RegisterDefaultZoomLevelCallback(
          base::Bind(&BraveBrowserContext::UpdateDefaultZoomLevel,
                     base::Unretained(this)));
}

ChromeZoomLevelPrefs* BraveBrowserContext::GetZoomLevelPrefs() {
  return static_cast<ChromeZoomLevelPrefs*>(
      GetDefaultStoragePartition(this)->GetZoomLevelDelegate());
}

void BraveBrowserContext::OnParentZoomLevelChanged(
    const HostZoomMap::ZoomLevelChange& change) {
  HostZoomMap* host_zoom_map = HostZoomMap::GetDefaultForBrowserContext(this);
  switch (change.mode) {
    case HostZoomMap::ZOOM_CHANGED_TEMPORARY_ZOOM:
      return;
    case HostZoomMap::ZOOM_CHANGED_FOR_HOST:
      host_zoom_map->SetZoomLevelForHost(change.host, change.zoom_level);
      return;
    case HostZoomMap::ZOOM_CHANGED_FOR_SCHEME_AND_HOST:
      host_zoom_map->SetZoomLevelForHostAndScheme(change.scheme,
          change.host,
          change.zoom_level);
      return;
    case HostZoomMap::PAGE_SCALE_IS_ONE_CHANGED:
      return;
  }
}

void BraveBrowserContext::UpdateDefaultZoomLevel() {
  HostZoomMap* host_zoom_map = HostZoomMap::GetDefaultForBrowserContext(this);
  double default_zoom_level =
      original_context_->GetZoomLevelPrefs()->GetDefaultZoomLevelPref();
  host_zoom_map->SetDefaultZoomLevel(default_zoom_level);
  // HostZoomMap does not trigger zoom notification events when the default
  // zoom level is set, so we need to do it here.
  zoom::ZoomEventManager::GetForBrowserContext(this)
      ->OnDefaultZoomLevelChanged();
}

content::PermissionManager* BraveBrowserContext::GetPermissionManager() {
  if (!permission_manager_.get())
    permission_manager_.reset(new BravePermissionManager);
  return permission_manager_.get();
}

atom::AtomNetworkDelegate* BraveBrowserContext::network_delegate() {
  auto getter = GetRequestContext();
  DCHECK(getter);
  return static_cast<atom::AtomNetworkDelegate*>(
      getter->GetURLRequestContext()->network_delegate());
}

net::URLRequestContextGetter* BraveBrowserContext::GetRequestContext() {
  return GetDefaultStoragePartition(this)->GetURLRequestContext();
}

net::NetworkDelegate* BraveBrowserContext::CreateNetworkDelegate() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return new extensions::AtomExtensionsNetworkDelegate(this);
}

std::unique_ptr<net::URLRequestJobFactory>
BraveBrowserContext::CreateURLRequestJobFactory(
    content::ProtocolHandlerMap* protocol_handlers) {
  std::unique_ptr<net::URLRequestJobFactory> job_factory =
      AtomBrowserContext::CreateURLRequestJobFactory(protocol_handlers);
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::InfoMap* extension_info_map =
      extensions::AtomExtensionSystemFactory::GetInstance()->
        GetForBrowserContext(this)->info_map();
  static_cast<atom::AtomURLRequestJobFactory*>(
      job_factory.get())->SetProtocolHandler(
          extensions::kExtensionScheme,
          extensions::CreateExtensionProtocolHandler(IsOffTheRecord(),
                                                     extension_info_map));
#endif
  protocol_handler_interceptor_->Chain(std::move(job_factory));
  return std::move(protocol_handler_interceptor_);
}

void BraveBrowserContext::CreateProfilePrefs(
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  InitPrefs(task_runner);
#if BUILDFLAG(ENABLE_EXTENSIONS)
  PrefStore* extension_prefs = new ExtensionPrefStore(
      ExtensionPrefValueMapFactory::GetForBrowserContext(original_context()),
      IsOffTheRecord());
#else
  PrefStore* extension_prefs = NULL;
#endif
  user_prefs_registrar_.reset(new PrefChangeRegistrar());

  bool async = false;

  if (IsOffTheRecord()) {
    overlay_pref_names_.push_back("app_state");
    overlay_pref_names_.push_back("content_settings");
    overlay_pref_names_.push_back(prefs::kPartitionPerHostZoomLevels);
    user_prefs_.reset(
        original_context()->user_prefs()->CreateIncognitoPrefService(
          extension_prefs, overlay_pref_names_));
    user_prefs::UserPrefs::Set(this, user_prefs_.get());
  } else if (HasParentContext()) {
    // overlay pref names only apply to incognito
    std::vector<const char*> overlay_pref_names;
    user_prefs_.reset(
            original_context()->user_prefs()->CreateIncognitoPrefService(
              extension_prefs, overlay_pref_names));
    user_prefs::UserPrefs::Set(this, user_prefs_.get());
  } else {
    pref_registry_->RegisterDictionaryPref("app_state");
    pref_registry_->RegisterDictionaryPref(prefs::kPartitionDefaultZoomLevel);
    pref_registry_->RegisterDictionaryPref(prefs::kPartitionPerHostZoomLevels);
    pref_registry_->RegisterBooleanPref(prefs::kPrintingEnabled, true);
    pref_registry_->RegisterBooleanPref(prefs::kPrintPreviewDisabled, false);
#if BUILDFLAG(ENABLE_PLUGINS)
    pref_registry_->RegisterBooleanPref(prefs::kPluginsAllowOutdated, false);
    pref_registry_->RegisterBooleanPref(prefs::kPluginsAlwaysAuthorize, false);
    PepperFlashSettingsManager::RegisterProfilePrefs(pref_registry_.get());
#endif
    // TODO(bridiver) - is this necessary or is it covered by
    // BrowserContextDependencyManager
    ProtocolHandlerRegistry::RegisterProfilePrefs(pref_registry_.get());
    HostContentSettingsMap::RegisterProfilePrefs(pref_registry_.get());
    autofill::AutofillManager::RegisterProfilePrefs(pref_registry_.get());
#if BUILDFLAG(ENABLE_EXTENSIONS)
    extensions::AtomBrowserClientExtensionsPart::RegisterProfilePrefs(
        pref_registry_.get());
    extensions::ExtensionPrefs::RegisterProfilePrefs(pref_registry_.get());
#endif
    BrowserContextDependencyManager::GetInstance()->
        RegisterProfilePrefsForServices(this, pref_registry_.get());

    // create profile prefs
    base::FilePath filepath = GetPath().Append(
        FILE_PATH_LITERAL("UserPrefs"));
    scoped_refptr<JsonPrefStore> pref_store =
        new JsonPrefStore(filepath, task_runner, std::unique_ptr<PrefFilter>());

    // prepare factory
    sync_preferences::PrefServiceSyncableFactory factory;
    factory.set_async(async);
    factory.set_extension_prefs(extension_prefs);
    factory.set_user_prefs(pref_store);
    user_prefs_ = factory.CreateSyncable(pref_registry_.get());
    user_prefs::UserPrefs::Set(this, user_prefs_.get());
    if (async) {
      user_prefs_->AddPrefInitObserver(base::Bind(
          &BraveBrowserContext::OnPrefsLoaded, base::Unretained(this)));
      return;
    }
  }

  OnPrefsLoaded(true);
}

void BraveBrowserContext::OnPrefsLoaded(bool success) {
  CHECK(success);

  BrowserContextDependencyManager::GetInstance()->
      CreateBrowserContextServices(this);

  protocol_handler_interceptor_ =
        ProtocolHandlerRegistryFactory::GetForBrowserContext(this)
          ->CreateJobInterceptorFactory();

  if (!IsOffTheRecord() && !HasParentContext()) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
    extensions::ExtensionSystem::Get(this)->InitForRegularProfile(true);
#endif
    content::BrowserContext::GetDefaultStoragePartition(this)->
        GetDOMStorageContext()->SetSaveSessionStorageOnDisk();

    // Initialize autofill db
    base::FilePath webDataPath = GetPath().Append(kWebDataFilename);

    CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    web_database_ = new WebDatabaseService(webDataPath,
        BrowserThread::GetTaskRunnerForThread(BrowserThread::UI),
        BrowserThread::GetTaskRunnerForThread(BrowserThread::DB));
    web_database_->AddTable(base::WrapUnique(new autofill::AutofillTable));
    web_database_->LoadDatabase();

    autofill_data_ = new autofill::AutofillWebDataService(
        web_database_,
        BrowserThread::GetTaskRunnerForThread(BrowserThread::UI),
        BrowserThread::GetTaskRunnerForThread(BrowserThread::DB),
        base::Bind(&DatabaseErrorCallback));
    autofill_data_->Init();
  }

  user_prefs_registrar_->Init(user_prefs_.get());
#if BUILDFLAG(ENABLE_PLUGINS)
  BravePluginServiceFilter::GetInstance()->RegisterResourceContext(
      this, GetResourceContext());
#endif

  ready_->Signal();
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_PROFILE_CREATED,
      content::Source<BraveBrowserContext>(this),
      content::NotificationService::NoDetails());
}

content::ResourceContext* BraveBrowserContext::GetResourceContext() {
  content::BrowserContext::EnsureResourceContextInitialized(this);
  return brightray::BrowserContext::GetResourceContext();
}

std::unique_ptr<content::ZoomLevelDelegate>
BraveBrowserContext::CreateZoomLevelDelegate(
    const base::FilePath& partition_path) {
  return base::WrapUnique(new ChromeZoomLevelPrefs(
      GetPrefs(), GetPath(), partition_path,
      zoom::ZoomEventManager::GetForBrowserContext(this)->GetWeakPtr()));
}

scoped_refptr<autofill::AutofillWebDataService>
BraveBrowserContext::GetAutofillWebdataService() {
  return original_context()->autofill_data_;
}

base::FilePath BraveBrowserContext::GetPath() const {
  return brightray::BrowserContext::GetPath();
}

std::string BraveBrowserContext::partition_with_prefix() {
  std::string canonical_partition(partition());
  if (canonical_partition.empty())
    canonical_partition = "default";

  if (IsOffTheRecord())
    return canonical_partition;

  return kPersistPrefix + canonical_partition;
}

scoped_refptr<atom::AtomBrowserContext> BraveBrowserContext::FromPartition(
    const std::string& partition, const base::DictionaryValue& options) {
  scoped_refptr<atom::AtomBrowserContext> browser_context;
  if (partition.empty()) {
    return atom::AtomBrowserContext::From("", false, options);
  }

  if (base::StartsWith(
      partition, kPersistPrefix, base::CompareCase::SENSITIVE)) {
    std::string name = partition.substr(kPersistPrefixLength);
    return atom::AtomBrowserContext::From(
        name == "default" ? "" : name, false, options);
  } else {
    return atom::AtomBrowserContext::From(
        partition == "default" ? "" : partition, true, options);
  }
}

}  // namespace brave

namespace atom {

void CreateDirectoryAndSignal(const base::FilePath& path,
                              base::WaitableEvent* done_creating) {
  if (!base::PathExists(path)) {
    DVLOG(1) << "Creating directory " << path.value();
    base::CreateDirectory(path);
  }
  done_creating->Signal();
}

// Task that blocks the FILE thread until CreateDirectoryAndSignal() finishes on
// blocking I/O pool.
void BlockFileThreadOnDirectoryCreate(base::WaitableEvent* done_creating) {
  done_creating->Wait();
}

// Initiates creation of profile directory on |sequenced_task_runner| and
// ensures that FILE thread is blocked until that operation finishes. If
// |create_readme| is true, the profile README will be created in the profile
// directory.
void CreateProfileDirectory(base::SequencedTaskRunner* sequenced_task_runner,
                            const base::FilePath& path) {
  base::WaitableEvent* done_creating =
      new base::WaitableEvent(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                              base::WaitableEvent::InitialState::NOT_SIGNALED);
  sequenced_task_runner->PostTask(
      FROM_HERE, base::Bind(&CreateDirectoryAndSignal, path, done_creating));
  // Block the FILE thread until directory is created on I/O pool to make sure
  // that we don't attempt any operation until that part completes.
  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&BlockFileThreadOnDirectoryCreate,
                 base::Owned(done_creating)));
}

// TODO(bridiver) find a better way to do this
// static
scoped_refptr<AtomBrowserContext> AtomBrowserContext::From(
    const std::string& partition, bool in_memory,
    const base::DictionaryValue& options) {
  auto browser_context = brightray::BrowserContext::Get(partition, in_memory);
  if (browser_context)
    return static_cast<AtomBrowserContext*>(browser_context.get());

  // TODO(bridiver) - pass the path to initialize the browser context
  // TODO(bridiver) - create these with the profile manager
  base::FilePath path;
  PathService::Get(brightray::DIR_USER_DATA, &path);
  if (!in_memory && !partition.empty())
    path = path.Append(FILE_PATH_LITERAL("Partitions"))
                 .Append(base::FilePath::FromUTF8Unsafe(
                      net::EscapePath(base::ToLowerASCII(partition))));

  // Get sequenced task runner for making sure that file operations of
  // this profile (defined by |path|) are executed in expected order
  // (what was previously assured by the FILE thread).
  scoped_refptr<base::SequencedTaskRunner> sequenced_task_runner =
      JsonPrefStore::GetTaskRunnerForFile(path,
                                          BrowserThread::GetBlockingPool());

  CreateProfileDirectory(sequenced_task_runner.get(), path);

  auto profile = new brave::BraveBrowserContext(partition, in_memory, options,
      sequenced_task_runner);

  if (profile == profile->GetOriginalProfile() &&
      !g_browser_process->profile_manager()->GetProfileByPath(
          profile->GetPath())) {
    g_browser_process->profile_manager()->AddProfile(profile);
  }

  return profile;
}

}  // namespace atom

