// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/http_protocol_handler.h"

#include "net/url_request/url_request_http_job.h"
#include "net/url_request/url_request_error_job.h"
#include "net/url_request/url_request_context.h"
#include "atom/common/node_bindings.h"
#include "atom/common/asar/archive.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "atom/common/asar/asar_util.h"

// #include <iostream>

#include "ABPFilterParser.h"
ABPFilterParser parser;
std::string binaryFilterData;

base::FilePath GetResourcesPath(bool is_browser) {
  auto command_line = base::CommandLine::ForCurrentProcess();
  base::FilePath exec_path(command_line->GetProgram());
  PathService::Get(base::FILE_EXE, &exec_path);

  base::FilePath resources_path =
#if defined(OS_MACOSX)
      is_browser ? exec_path.DirName().DirName().Append("Resources") :
                   exec_path.DirName().DirName().DirName().DirName().DirName()
                            .Append("Resources");
#else
      exec_path.DirName().Append(FILE_PATH_LITERAL("resources"));
#endif
  return resources_path;
}


namespace atom {

HttpProtocolHandler::HttpProtocolHandler(const std::string& scheme)
    : scheme_(scheme) {
  // Read the binary data from within the data.asar file
  base::FilePath resources_path = GetResourcesPath(true);
  base::FilePath data_path =
    resources_path.Append(
        FILE_PATH_LITERAL("data.asar/ABPFilterParserData.dat"));
  asar::ReadFileToString(data_path, &binaryFilterData);
  parser.deserialize(&binaryFilterData.front());
}

HttpProtocolHandler::~HttpProtocolHandler() {
}

net::URLRequestJob* HttpProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {

  FilterOption filterOption = FONoFilterOption;
  std::string accept;
  if (request->extra_request_headers().GetHeader("Accept", &accept)) {
    if (accept.find("/css") != std::string::npos) {
      // cout << "stylesheet!" << endl;
      filterOption = FOStylesheet;
    } else if (accept.find("image/") != std::string::npos) {
      filterOption = FOImage;
      // std::cout << "image!" << std::endl;
    } else if (accept.find("javascript") != std::string::npos) {
      filterOption = FOScript;
      // std::cout << "javascript!" << std::endl;
    }
  }

  // std::cout << "1. third party: "
  // << request->first_party_for_cookies().host() << std::endl;
  // std::cout << "2. url: " << request->url().host() << std::endl;
  // std::cout << "3. accept: " << accept.c_str() << std::endl;

  if (filterOption != FOStylesheet &&
      parser.matches(request->url().spec().c_str(),
        filterOption,
        request->first_party_for_cookies().host().c_str())) {
    // std::cout << "BLOCK: " << request->url().spec().c_str() << std::endl;
    return new net::URLRequestErrorJob(request,
      network_delegate, net::ERR_FAILED);
  } else {
    // std::cout << "NotMatch: ABPFilterParser: "
    //   << request->url().spec().c_str() << std::endl;
  }

  return net::URLRequestHttpJob::Factory(request,
                                         network_delegate,
                                         scheme_);
}

}  // namespace atom
