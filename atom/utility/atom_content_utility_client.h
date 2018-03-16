// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_
#define ATOM_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_

#include <memory>
#include <vector>

#include "build/build_config.h"
#include "content/public/utility/content_utility_client.h"
#include "printing/features/features.h"

namespace base {
class FilePath;
struct FileDescriptor;
}

namespace printing {
class PrintingHandler;
}

namespace shell {
class InterfaceRegistry;
}

namespace atom {

class AtomContentUtilityClient : public content::ContentUtilityClient {
 public:
  AtomContentUtilityClient();
  ~AtomContentUtilityClient() override;

  // content::ContentUtilityClient:
  void UtilityThreadStarted() override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void RegisterServices(StaticServiceMap* services) override;

  static void PreSandboxStartup();

 private:
#if defined(OS_WIN) && BUILDFLAG(ENABLE_PRINT_PREVIEW)
  // Last IPC message handler.
  std::unique_ptr<printing::PrintingHandler> printing_handler_;
#endif

  // True if the utility process runs with elevated privileges.
  bool utility_process_running_elevated_;

  DISALLOW_COPY_AND_ASSIGN(AtomContentUtilityClient);
};

}  // namespace atom

#endif  // ATOM_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_
