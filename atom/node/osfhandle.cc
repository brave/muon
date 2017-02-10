// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "osfhandle.h"

#include <io.h>

using v8_inspector::StringView;
using v8_inspector::StringBuffer;
using v8_inspector::protocol::Runtime::API::RemoteObject;

namespace node {

// TODO(bridiver) - this method is just a huge hack to ensure that these
// symbols get exported in node.dll
void symbol_hack(char* reason) {
  StringView test;
  if (reason ==
      v8_inspector::protocol::Debugger::API::Paused::ReasonEnum::Other &&
      v8_inspector::V8InspectorSession::canDispatchMethod(test)) {
    RemoteObject::fromJSONString(test);
    v8::TracingCpuProfiler::Create(NULL);
  }
}

int open_osfhandle(intptr_t osfhandle, int flags) {
  return _open_osfhandle(osfhandle, flags);
}

int close(int fd) {
  return _close(fd);
}

}  // namespace node
