// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_browser_client_extensions_part.h"

#include <map>
#include <set>

#include "atom/common/api/api_messages.h"
#include "base/command_line.h"
#include "brave/browser/api/brave_api_extension.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/renderer_host/chrome_extension_message_filter.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browser_url_handler.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_switches.h"
#include "extensions/browser/api/web_request/web_request_api.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_message_filter.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_service_worker_message_filter.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/guest_view/extensions_guest_view_message_filter.h"
#include "extensions/browser/info_map.h"
#include "extensions/browser/io_thread_extension_message_filter.h"
#include "extensions/browser/process_manager.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_l10n_util.h"
#include "extensions/common/extensions_client.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/manifest_handlers/web_accessible_resources_info.h"
#include "extensions/common/switches.h"

using content::BrowserContext;
using content::BrowserThread;
using content::BrowserURLHandler;
using content::SiteInstance;

namespace extensions {

namespace {

enum RenderProcessHostPrivilege {
  PRIV_NORMAL,
  PRIV_EXTENSION,
};

RenderProcessHostPrivilege GetPrivilegeRequiredByUrl(
    const GURL& url,
    ExtensionRegistry* registry) {
  // Default to a normal renderer cause it is lower privileged. This should only
  // occur if the URL on a site instance is either malformed, or uninitialized.
  // If it is malformed, then there is no need for better privileges anyways.
  // If it is uninitialized, but eventually settles on being an a scheme other
  // than normal webrenderer, the navigation logic will correct us out of band
  // anyways.
  if (!url.is_valid())
    return PRIV_NORMAL;

  if (!url.SchemeIs(kExtensionScheme))
    return PRIV_NORMAL;

  return PRIV_EXTENSION;
}

RenderProcessHostPrivilege GetProcessPrivilege(
    content::RenderProcessHost* process_host,
    ProcessMap* process_map,
    ExtensionRegistry* registry) {
  std::set<std::string> extension_ids =
      process_map->GetExtensionsInProcess(process_host->GetID());
  if (extension_ids.empty())
    return PRIV_NORMAL;

  return PRIV_EXTENSION;
}

static std::map<int, void*> render_process_hosts_;

}  // namespace

AtomBrowserClientExtensionsPart::AtomBrowserClientExtensionsPart() {
}

AtomBrowserClientExtensionsPart::~AtomBrowserClientExtensionsPart() {
}

// static
bool AtomBrowserClientExtensionsPart::ShouldUseProcessPerSite(
    Profile* profile, const GURL& effective_url) {
  // return false for non-extension urls
  if (!effective_url.SchemeIs(kExtensionScheme))
    return false;

  ExtensionRegistry* registry = ExtensionRegistry::Get(profile);
  if (!registry)
    return false;

  const Extension* extension =
      registry->enabled_extensions().GetByID(effective_url.host());
  if (!extension)
    return false;

  return true;
}

// static
bool AtomBrowserClientExtensionsPart::DoesSiteRequireDedicatedProcess(
    content::BrowserContext* browser_context,
    const GURL& effective_site_url) {
  return false;
}

// static
bool AtomBrowserClientExtensionsPart::ShouldLockToOrigin(
    content::BrowserContext* browser_context,
    const GURL& effective_site_url) {
  // https://crbug.com/160576 workaround: Origin lock to the chrome-extension://
  // scheme for a hosted app would kill processes on legitimate requests for the
  // app's cookies.
  if (effective_site_url.SchemeIs(extensions::kExtensionScheme)) {
    // http://crbug.com/600441 workaround: Extension process reuse, implemented
    // in ShouldTryToUseExistingProcessHost(), means that extension processes
    // aren't always actually dedicated to a single origin, even in
    // --isolate-extensions. TODO(nick): Fix this.
    if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
            ::switches::kSitePerProcess))
      return false;
  }
  return true;
}

// static
bool AtomBrowserClientExtensionsPart::IsSuitableHost(
    Profile* profile,
    content::RenderProcessHost* process_host,
    const GURL& site_url) {
  DCHECK(profile);

  ExtensionRegistry* registry = ExtensionRegistry::Get(profile);
  ProcessMap* process_map = ProcessMap::Get(profile);

  // These may be NULL during tests. In that case, just assume any site can
  // share any host.
  if (!registry || !process_map)
    return true;

  // Otherwise, just make sure the process privilege matches the privilege
  // required by the site.
  RenderProcessHostPrivilege privilege_required =
      GetPrivilegeRequiredByUrl(site_url, registry);
  return GetProcessPrivilege(process_host, process_map, registry) ==
         privilege_required;
}

// static
bool
AtomBrowserClientExtensionsPart::ShouldTryToUseExistingProcessHost(
    Profile* profile, const GURL& url) {
  // This function is trying to limit the amount of processes used by extensions
  // with background pages. It uses a globally set percentage of processes to
  // run such extensions and if the limit is exceeded, it returns true, to
  // indicate to the content module to group extensions together.
  ExtensionRegistry* registry =
      profile ? ExtensionRegistry::Get(profile) : NULL;
  if (!registry)
    return false;

  // We have to have a valid extension with background page to proceed.
  const Extension* extension =
      registry->enabled_extensions().GetExtensionOrAppByURL(url);
  if (!extension)
    return false;
  if (!BackgroundInfo::HasBackgroundPage(extension))
    return false;

  std::set<int> process_ids;
  size_t max_process_count =
      content::RenderProcessHost::GetMaxRendererProcessCount();

  // Go through all profiles to ensure we have total count of extension
  // processes containing background pages, otherwise one profile can
  // starve the other.
  std::vector<Profile*> profiles = g_browser_process->profile_manager()->
      GetLoadedProfiles();
  for (size_t i = 0; i < profiles.size(); ++i) {
    ProcessManager* epm = ProcessManager::Get(profiles[i]);
    for (ExtensionHost* host : epm->background_hosts())
      process_ids.insert(host->render_process_host()->GetID());
  }

  return (process_ids.size() >
          (max_process_count * chrome::kMaxShareOfExtensionProcesses));
}

// static
bool AtomBrowserClientExtensionsPart::
    ShouldSwapBrowsingInstancesForNavigation(SiteInstance* site_instance,
                                             const GURL& current_url,
                                             const GURL& new_url) {
  // If we don't have an ExtensionRegistry, then rely on the SiteInstance logic
  // in RenderFrameHostManager to decide when to swap.
  ExtensionRegistry* registry =
      ExtensionRegistry::Get(site_instance->GetBrowserContext());
  if (!registry)
    return false;

  // We must use a new BrowsingInstance (forcing a process swap and disabling
  // scripting by existing tabs) if one of the URLs is an extension and the
  // other is not the exact same extension.
  const Extension* current_extension =
      registry->enabled_extensions().GetExtensionOrAppByURL(current_url);
  if (current_extension && current_extension->is_hosted_app())
    current_extension = NULL;

  const Extension* new_extension =
      registry->enabled_extensions().GetExtensionOrAppByURL(new_url);
  if (new_extension && new_extension->is_hosted_app())
    new_extension = NULL;

  // First do a process check.  We should force a BrowsingInstance swap if the
  // current process doesn't know about new_extension, even if current_extension
  // is somehow the same as new_extension.
  ProcessMap* process_map = ProcessMap::Get(site_instance->GetBrowserContext());
  if (new_extension &&
      site_instance->HasProcess() &&
      !process_map->Contains(
          new_extension->id(), site_instance->GetProcess()->GetID()))
    return true;

  // Otherwise, swap BrowsingInstances if current_extension and new_extension
  // differ.
  return current_extension != new_extension;
}

// static
bool AtomBrowserClientExtensionsPart::ShouldAllowOpenURL(
    content::SiteInstance* site_instance,
    const GURL& from_url,
    const GURL& to_url,
    bool* result) {
  DCHECK(result);

  // Do not allow pages from the web or other extensions navigate to
  // non-web-accessible extension resources.
  if (to_url.SchemeIs(kExtensionScheme) &&
      (from_url.SchemeIsHTTPOrHTTPS() || from_url.SchemeIs(kExtensionScheme))) {
    content::BrowserContext* browser_context =
        site_instance->GetProcess()->GetBrowserContext();
    ExtensionRegistry* registry = ExtensionRegistry::Get(browser_context);
    if (!registry) {
      *result = true;
      return true;
    }
    const Extension* extension =
        registry->enabled_extensions().GetExtensionOrAppByURL(to_url);
    if (!extension) {
      *result = true;
      return true;
    }
    const Extension* from_extension =
        registry->enabled_extensions().GetExtensionOrAppByURL(
            site_instance->GetSiteURL());
    if (from_extension && from_extension->id() == extension->id()) {
      *result = true;
      return true;
    }

    if (!WebAccessibleResourcesInfo::IsResourceWebAccessible(
            extension, to_url.path())) {
      *result = false;
      return true;
    }
  }
  return false;
}

std::string AtomBrowserClientExtensionsPart::GetApplicationLocale() {
  return extension_l10n_util::CurrentLocaleOrDefault();
}

// static
void AtomBrowserClientExtensionsPart::SetApplicationLocale(std::string locale) {
  extension_l10n_util::SetProcessLocale(locale);
}

// static
void AtomBrowserClientExtensionsPart::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kApplicationLocale,
      extension_l10n_util::CurrentLocaleOrDefault());
}

void AtomBrowserClientExtensionsPart::OverrideWebkitPrefs(
    content::RenderViewHost* host, content::WebPreferences* prefs) {
  host->Send(new AtomMsg_UpdateWebKitPrefs(*prefs));
}

// static
bool AtomBrowserClientExtensionsPart::
    DoesOriginMatchAllURLsInWebExtent(const url::Origin& origin,
                                      const URLPatternSet& web_extent) {
  // This function assumes |origin| is an isolated origin, which can only have
  // an HTTP or HTTPS scheme (see IsolatedOriginUtil::IsValidIsolatedOrigin()),
  // so these are the only schemes allowed to be matched below.
  DCHECK(origin.scheme() == url::kHttpsScheme ||
         origin.scheme() == url::kHttpScheme);
  URLPattern origin_pattern(URLPattern::SCHEME_HTTPS | URLPattern::SCHEME_HTTP);
  // TODO(alexmos): Temporarily disable precise scheme matching on
  // |origin_pattern| to allow apps that use *://foo.com/ in their web extent
  // to still work with isolated origins.  See https://crbug.com/799638.  We
  // should use SetScheme(origin.scheme()) here once https://crbug.com/791796
  // is fixed.
  origin_pattern.SetScheme("*");
  origin_pattern.SetHost(origin.host());
  origin_pattern.SetPath("/*");
  // We allow matching subdomains here because |origin| is the precise origin
  // retrieved from site isolation policy. Thus, we'll only allow an extent of
  // foo.example.com and bar.example.com if the isolated origin was
  // example.com; if the isolated origin is foo.example.com, this will
  // correctly fail.
  origin_pattern.SetMatchSubdomains(true);

  URLPatternSet origin_pattern_list;
  origin_pattern_list.AddPattern(origin_pattern);
  return origin_pattern_list.Contains(web_extent);
}

void AtomBrowserClientExtensionsPart::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  int id = host->GetID();
  auto context =
      Profile::FromBrowserContext(host->GetBrowserContext());

  host->AddFilter(new ChromeExtensionMessageFilter(id, context));
  host->AddFilter(new ExtensionMessageFilter(id, context));
  host->AddFilter(new IOThreadExtensionMessageFilter(id, context));
  host->AddFilter(new ExtensionsGuestViewMessageFilter(id, context));
  if (extensions::ExtensionsClient::Get()
          ->ExtensionAPIEnabledInExtensionServiceWorkers()) {
    host->AddFilter(new ExtensionServiceWorkerMessageFilter(
       id, context, host->GetStoragePartition()->GetServiceWorkerContext()));
  }

  auto user_prefs_registrar = context->user_prefs_change_registrar();
  if (!user_prefs_registrar->IsObserved("content_settings")) {
    user_prefs_registrar->Add(
        "content_settings",
        base::Bind(&AtomBrowserClientExtensionsPart::UpdateContentSettings,
                   base::Unretained(this)));
  }
  UpdateContentSettingsForHost(host->GetID());
}

// static
GURL AtomBrowserClientExtensionsPart::GetEffectiveURL(
    Profile* profile,
    const GURL& url) {
  // If the input |url| is part of an installed app, the effective URL is an
  // extension URL with the ID of that extension as the host. This has the
  // effect of grouping apps together in a common SiteInstance.
  ExtensionRegistry* registry = ExtensionRegistry::Get(profile);
  if (!registry)
    return url;

  const Extension* extension =
      registry->enabled_extensions().GetHostedAppByURL(url);
  if (!extension)
    return url;

  // Bookmark apps do not use the hosted app process model, and should be
  // treated as normal URLs.
  if (extension->from_bookmark())
    return url;

  // If |url| corresponds to both an isolated origin and a hosted app,
  // determine whether to use the effective URL, which also determines whether
  // the isolated origin should take precedence over a matching hosted app:
  // - Chrome Web Store should always be resolved to its effective URL, so that
  //   the CWS process gets proper bindings.
  // - for other hosted apps, if the isolated origin covers the app's entire
  //   web extent (i.e., *all* URLs matched by the hosted app will have this
  //   isolated origin), allow the hosted app to take effect and return an
  //   effective URL.
  // - for other cases, disallow effective URLs, as otherwise this would allow
  //   the isolated origin to share the hosted app process with other origins
  //   it does not trust, due to https://crbug.com/791796.
  //
  // TODO(alexmos): Revisit and possibly remove this once
  // https://crbug.com/791796 is fixed.
  url::Origin isolated_origin;
  auto* policy = content::ChildProcessSecurityPolicy::GetInstance();
  bool is_isolated_origin = policy->GetMatchingIsolatedOrigin(
      url::Origin::Create(url), &isolated_origin);
  if (is_isolated_origin && extension->id() != kWebStoreAppId &&
      !DoesOriginMatchAllURLsInWebExtent(isolated_origin,
                                         extension->web_extent())) {
    return url;
  }


  // If the URL is part of an extension's web extent, convert it to an
  // extension URL.
  return extension->GetResourceURL(url.path());
}

void AtomBrowserClientExtensionsPart::UpdateContentSettingsForHost(
  int render_process_id) {
  auto host = content::RenderProcessHost::FromID(render_process_id);
  if (!host)
    return;

  auto context =
      static_cast<atom::AtomBrowserContext*>(host->GetBrowserContext());
  auto user_prefs = user_prefs::UserPrefs::Get(context);
  const base::DictionaryValue* content_settings =
    user_prefs->GetDictionary("content_settings");
  host->Send(new AtomMsg_UpdateContentSettings(*content_settings));
}

void AtomBrowserClientExtensionsPart::UpdateContentSettings() {
  for (std::map<int, void*>::iterator
      it = render_process_hosts_.begin();
      it != render_process_hosts_.end();
      ++it) {
    UpdateContentSettingsForHost(it->first);
  }
}

void AtomBrowserClientExtensionsPart::SiteInstanceGotProcess(
    SiteInstance* site_instance) {
  BrowserContext* context = site_instance->GetProcess()->GetBrowserContext();
  ExtensionRegistry* registry = ExtensionRegistry::Get(context);
  if (!registry)
    return;

  render_process_hosts_[site_instance->GetProcess()->GetID()] = nullptr;

  const Extension* extension =
      registry->enabled_extensions().GetExtensionOrAppByURL(
          site_instance->GetSiteURL());
  if (!extension)
    return;

  ProcessMap::Get(context)->Insert(extension->id(),
                                   site_instance->GetProcess()->GetID(),
                                   site_instance->GetId());

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&InfoMap::RegisterExtensionProcess,
                 ExtensionSystem::Get(context)->info_map(), extension->id(),
                 site_instance->GetProcess()->GetID(), site_instance->GetId()));
}

void AtomBrowserClientExtensionsPart::SiteInstanceDeleting(
    SiteInstance* site_instance) {
  BrowserContext* context = site_instance->GetBrowserContext();
  ExtensionRegistry* registry = ExtensionRegistry::Get(context);
  if (!registry)
    return;

  if (render_process_hosts_[site_instance->GetProcess()->GetID()])
    render_process_hosts_.erase(site_instance->GetProcess()->GetID());

  const Extension* extension =
      registry->enabled_extensions().GetExtensionOrAppByURL(
          site_instance->GetSiteURL());
  if (!extension)
    return;

  ProcessMap::Get(context)->Remove(extension->id(),
                                   site_instance->GetProcess()->GetID(),
                                   site_instance->GetId());

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&InfoMap::UnregisterExtensionProcess,
                 ExtensionSystem::Get(context)->info_map(), extension->id(),
                 site_instance->GetProcess()->GetID(), site_instance->GetId()));
}

void AtomBrowserClientExtensionsPart::BrowserURLHandlerCreated(
    BrowserURLHandler* handler) {
  handler->AddHandlerPair(&brave::api::Extension::HandleURLOverride,
                          &brave::api::Extension::HandleURLOverrideReverse);
}

void AtomBrowserClientExtensionsPart::
    GetAdditionalAllowedSchemesForFileSystem(
        std::vector<std::string>* additional_allowed_schemes) {
  additional_allowed_schemes->push_back(kExtensionScheme);
}

void AtomBrowserClientExtensionsPart::
    AppendExtraRendererCommandLineSwitches(base::CommandLine* command_line,
                                           content::RenderProcessHost* process,
                                           BrowserContext* context) {
  if (!process)
    return;
  DCHECK(context);
  if (ProcessMap::Get(context)->Contains(process->GetID())) {
    command_line->AppendSwitch(switches::kExtensionProcess);
  }
}

}  // namespace extensions
