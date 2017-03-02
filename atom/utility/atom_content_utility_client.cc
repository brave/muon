// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <utility>

#include "atom/utility/atom_content_utility_client.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "chrome/common/chrome_utility_messages.h"
#include "chrome/common/resource_usage_reporter.mojom.h"
#include "chrome/utility/profile_import_handler.h"
#include "chrome/utility/utility_message_handler.h"
#include "content/public/common/content_switches.h"
#include "content/public/utility/utility_thread.h"
#include "extensions/features/features.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_message_macros.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/proxy/mojo_proxy_resolver_factory_impl.h"
#include "net/proxy/proxy_resolver_v8.h"
#include "printing/features/features.h"
#include "services/service_manager/public/cpp/interface_registry.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/common/extensions/chrome_extensions_client.h"
#include "extensions/utility/utility_handler.h"
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) || \
    (BUILDFLAG(ENABLE_BASIC_PRINTING) && defined(OS_WIN))
#include "chrome/utility/printing_handler.h"
#endif

namespace atom {

namespace {

void CreateProxyResolverFactory(
  net::interfaces::ProxyResolverFactoryRequest request) {
    mojo::MakeStrongBinding(
        base::MakeUnique<net::MojoProxyResolverFactoryImpl>(),
        std::move(request));
}

class ResourceUsageReporterImpl : public chrome::mojom::ResourceUsageReporter {
 public:
  ResourceUsageReporterImpl() {}
  ~ResourceUsageReporterImpl() override {}

 private:
  void GetUsageData(const GetUsageDataCallback& callback) override {
    chrome::mojom::ResourceUsageDataPtr data =
      chrome::mojom::ResourceUsageData::New();
    size_t total_heap_size = net::ProxyResolverV8::GetTotalHeapSize();
    if (total_heap_size) {
      data->reports_v8_stats = true;
      data->v8_bytes_allocated = total_heap_size;
      data->v8_bytes_used = net::ProxyResolverV8::GetUsedHeapSize();
    }
    callback.Run(std::move(data));
  }
};

void CreateResourceUsageReporter(
    mojo::InterfaceRequest<chrome::mojom::ResourceUsageReporter> request) {
  mojo::MakeStrongBinding(base::MakeUnique<ResourceUsageReporterImpl>(),
                          std::move(request));
}

}  // namespace

int64_t AtomContentUtilityClient::max_ipc_message_size_ =
    IPC::Channel::kMaximumMessageSize;

AtomContentUtilityClient::AtomContentUtilityClient()
    : filter_messages_(false) {
#if BUILDFLAG(ENABLE_PRINT_PREVIEW) || \
    (BUILDFLAG(ENABLE_BASIC_PRINTING) && defined(OS_WIN))
  handlers_.push_back(new printing::PrintingHandler());
#endif
}

AtomContentUtilityClient::~AtomContentUtilityClient() {
}

void AtomContentUtilityClient::UtilityThreadStarted() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::UtilityHandler::UtilityThreadStarted();
#endif
}

bool AtomContentUtilityClient::OnMessageReceived(
    const IPC::Message& message) {
  if (filter_messages_ &&
      !base::ContainsKey(message_id_whitelist_, message.type()))
    return false;

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AtomContentUtilityClient, message)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  if (handled)
    return true;

  for (auto* handler : handlers_) {
    // At least one of the utility process handlers adds a new handler to
    // |handlers_| when it handles a message. This causes any iterator over
    // |handlers_| to become invalid. Therefore, it is necessary to break the
    // loop at this point instead of evaluating it as a loop condition (if the
    // for loop was using iterators explicitly, as originally done).
    if (handler->OnMessageReceived(message))
      return true;
  }

  return false;
}

void AtomContentUtilityClient::ExposeInterfacesToBrowser(
    service_manager::InterfaceRegistry* registry) {
  // When the utility process is running with elevated privileges, we need to
  // filter messages so that only a whitelist of IPCs can run. In Mojo, there's
  // no way of filtering individual messages. Instead, we can avoid adding
  // non-whitelisted Mojo services to the service_manager::InterfaceRegistry.
  // TODO(amistry): Use a whitelist once the whistlisted IPCs have been
  // converted to Mojo.
  if (filter_messages_)
    return;

  registry->AddInterface<net::interfaces::ProxyResolverFactory>(
      base::Bind(CreateProxyResolverFactory));
  registry->AddInterface(base::Bind(CreateResourceUsageReporter));
  registry->AddInterface(base::Bind(&ProfileImportHandler::Create));
}

// static
void AtomContentUtilityClient::PreSandboxStartup() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::ExtensionsClient::Set(
      extensions::ChromeExtensionsClient::GetInstance());
#endif
}

}  // namespace atom
