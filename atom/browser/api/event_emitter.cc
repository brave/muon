// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/event_emitter.h"

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/api/event.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "brave/common/extensions/shared_memory_bindings.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "native_mate/arguments.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "ui/events/event_constants.h"

#include "atom/common/node_includes.h"

using atom::api::WebContents;

namespace mate {

namespace {

v8::Persistent<v8::ObjectTemplate> event_template;

void PreventDefault(mate::Arguments* args) {
  mate::Dictionary self(args->isolate(), args->GetThis());
  self.Set("defaultPrevented", true);
}

// Create a pure JavaScript Event object.
v8::Local<v8::Object> CreateEventObject(v8::Isolate* isolate) {
  if (event_template.IsEmpty()) {
    event_template.Reset(isolate, ObjectTemplateBuilder(isolate)
        .SetMethod("preventDefault", &PreventDefault)
        .Build());
  }

  return v8::Local<v8::ObjectTemplate>::New(
      isolate, event_template)->NewInstance();
}

}  // namespace

namespace internal {

v8::Local<v8::Object> CreateJSEvent(
    v8::Isolate* isolate,
    v8::Local<v8::Object> object,
    content::RenderFrameHost* render_frame_host,
    IPC::Message* message) {
  v8::Local<v8::Object> event;

  if (render_frame_host && message) {
    mate::Handle<mate::Event> native_event = mate::Event::Create(isolate);
    native_event->SetSenderAndMessage(render_frame_host, message);
    event = v8::Local<v8::Object>::Cast(native_event.ToV8());
  } else {
    event = CreateEventObject(isolate);

    if (render_frame_host) {
      v8::EscapableHandleScope handle_scope(isolate);
      auto web_contents =
          content::WebContents::FromRenderFrameHost(render_frame_host);
      // create a new wrapper so we can rebind send and sendShared
      // without affecting other references
      mate::Handle<WebContents> handle = WebContents::CreateFrom(
          isolate, web_contents, WebContents::Type::REMOTE);

      int render_process_id = render_frame_host->GetProcess()->GetID();
      int render_frame_id = render_frame_host->GetRoutingID();

      mate::Dictionary sender(isolate, handle->GetWrapper());
      sender.SetMethod("_send",
          base::Bind(&atom::api::WebContents::SendIPCMessage,
              render_process_id, render_frame_id));
      sender.SetMethod("_sendShared",
          base::Bind(&atom::api::WebContents::SendIPCSharedMemory,
              render_process_id, render_frame_id));

      if (render_frame_host != web_contents->GetMainFrame()) {
        auto mainFrame =
            WebContents::CreateFrom(isolate, web_contents)->GetWrapper();
        sender.Set("mainFrame", mainFrame);
      }

      object = handle_scope.Escape(handle.ToV8());
    }
  }
  mate::Dictionary(isolate, event).Set("sender", object);
  return event;
}

v8::Local<v8::Object> CreateCustomEvent(
    v8::Isolate* isolate,
    v8::Local<v8::Object> object,
    v8::Local<v8::Object> custom_event) {
  v8::Local<v8::Object> event = CreateEventObject(isolate);
  (void)event->SetPrototype(custom_event->CreationContext(), custom_event);
  mate::Dictionary(isolate, event).Set("sender", object);
  return event;
}

v8::Local<v8::Object> CreateEventFromFlags(v8::Isolate* isolate, int flags) {
  mate::Dictionary obj = mate::Dictionary::CreateEmpty(isolate);
  obj.Set("shiftKey", static_cast<bool>(flags & ui::EF_SHIFT_DOWN));
  obj.Set("ctrlKey", static_cast<bool>(flags & ui::EF_CONTROL_DOWN));
  obj.Set("altKey", static_cast<bool>(flags & ui::EF_ALT_DOWN));
  obj.Set("metaKey", static_cast<bool>(flags & ui::EF_COMMAND_DOWN));
  return obj.GetHandle();
}

}  // namespace internal

}  // namespace mate
