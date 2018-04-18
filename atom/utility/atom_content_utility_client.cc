// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <utility>

#include "atom/utility/atom_content_utility_client.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "brave/utility/importer/brave_profile_import_service.h"
#include "chrome/common/importer/profile_import.mojom.h"
#include "chrome/utility/extensions/extensions_handler.h"
#include "components/unzip_service/public/interfaces/constants.mojom.h"
#include "components/unzip_service/unzip_service.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/simple_connection_filter.h"
#include "content/public/utility/utility_thread.h"
#include "extensions/buildflags/buildflags.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_message_macros.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/proxy_resolver/proxy_resolver_service.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/sandbox/switches.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/common/extensions/chrome_extensions_client.h"
#include "extensions/utility/utility_handler.h"
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "chrome/services/printing/printing_service.h"
#include "chrome/services/printing/public/mojom/constants.mojom.h"
#endif

#if defined(OS_WIN) && BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "chrome/utility/printing_handler.h"
#endif

#if defined(OS_WIN)
#include "chrome/services/printing/pdf_to_emf_converter_factory.h"
#endif

namespace atom {

AtomContentUtilityClient::AtomContentUtilityClient()
    : utility_process_running_elevated_(false) {
#if defined(OS_WIN) && BUILDFLAG(ENABLE_PRINT_PREVIEW)
  printing_handler_ = std::make_unique<printing::PrintingHandler>();
#endif
}

AtomContentUtilityClient::~AtomContentUtilityClient() = default;

void AtomContentUtilityClient::UtilityThreadStarted() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::utility_handler::UtilityThreadStarted();
#endif

#if defined(OS_WIN)
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  utility_process_running_elevated_ = command_line->HasSwitch(
      service_manager::switches::kNoSandboxAndElevatedPrivileges);
#endif

  content::ServiceManagerConnection* connection =
      content::ChildThread::Get()->GetServiceManagerConnection();

  // NOTE: Some utility process instances are not connected to the Service
  // Manager. Nothing left to do in that case.
  if (!connection)
    return;

  auto registry = base::MakeUnique<service_manager::BinderRegistry>();
  // If our process runs with elevated privileges, only add elevated Mojo
  // interfaces to the interface registry.
  if (!utility_process_running_elevated_) {
#if defined(OS_WIN)
        // TODO(crbug.com/798782): remove when the Cloud print chrome/service is
        // removed.
    registry->AddInterface(
        base::Bind(printing::PdfToEmfConverterFactory::Create),
        base::ThreadTaskRunnerHandle::Get());
#endif
  }

  connection->AddConnectionFilter(
      base::MakeUnique<content::SimpleConnectionFilter>(std::move(registry)));
}

bool AtomContentUtilityClient::OnMessageReceived(
    const IPC::Message& message) {
  if (utility_process_running_elevated_)
    return false;

#if defined(OS_WIN) && BUILDFLAG(ENABLE_PRINT_PREVIEW)
  if (printing_handler_->OnMessageReceived(message))
      return true;
#endif
  return false;
}

void AtomContentUtilityClient::RegisterServices(
    AtomContentUtilityClient::StaticServiceMap* services) {
  service_manager::EmbeddedServiceInfo proxy_resolver_info;
  proxy_resolver_info.task_runner =
      content::ChildThread::Get()->GetIOTaskRunner();
  proxy_resolver_info.factory =
      base::Bind(&proxy_resolver::ProxyResolverService::CreateService);
  services->emplace(proxy_resolver::mojom::kProxyResolverServiceName,
                    proxy_resolver_info);

  service_manager::EmbeddedServiceInfo profile_import_info;
  profile_import_info.factory =
    base::Bind(&BraveProfileImportService::CreateService);
  services->emplace(chrome::mojom::kProfileImportServiceName,
                    profile_import_info);
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  {
    service_manager::EmbeddedServiceInfo printing_info;
    printing_info.factory =
      base::Bind(&printing::PrintingService::CreateService);
    services->emplace(printing::mojom::kChromePrintingServiceName,
                      printing_info);
  }
#endif
  {
    service_manager::EmbeddedServiceInfo service_info;
    service_info.factory =
        base::BindRepeating(&unzip::UnzipService::CreateService);
    services->emplace(unzip::mojom::kServiceName, service_info);
  }
}

// static
void AtomContentUtilityClient::PreSandboxStartup() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::ExtensionsClient::Set(
      extensions::ChromeExtensionsClient::GetInstance());
#endif
}

}  // namespace atom
