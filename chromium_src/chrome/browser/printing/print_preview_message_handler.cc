// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_preview_message_handler.h"

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/shared_memory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/printing/print_view_manager.h"
#include "chrome/browser/printing/print_view_manager_common.h"
#include "chrome/browser/printing/printer_query.h"
#include "components/printing/common/print_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "printing/page_size_margins.h"
#include "printing/pdf_metafile_skia.h"
#include "printing/print_job_constants.h"
#include "printing/units.h"

#include "atom/common/node_includes.h"

using content::BrowserThread;
using content::WebContents;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(printing::PrintPreviewMessageHandler);

namespace printing {

namespace {

void StopWorker(int document_cookie) {
  if (document_cookie <= 0)
    return;
  scoped_refptr<PrintQueriesQueue> queue =
      g_browser_process->print_job_manager()->queue();
  scoped_refptr<PrinterQuery> printer_query =
      queue->PopPrinterQuery(document_cookie);
  if (printer_query.get()) {
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                            base::Bind(&PrinterQuery::StopWorker,
                                       printer_query));
  }
}

char* CopyPDFDataOnIOThread(
    const PrintHostMsg_DidPreviewDocument_Params& params) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::unique_ptr<base::SharedMemory> shared_buf(
      new base::SharedMemory(params.metafile_data_handle, true));
  if (!shared_buf->Map(params.data_size))
    return nullptr;

  char* memory_pdf_data = static_cast<char*>(shared_buf->memory());
  char* pdf_data = new char[params.data_size];
  memcpy(pdf_data, memory_pdf_data, params.data_size);
  return pdf_data;
}

std::pair<int, int> GetKey(content::RenderFrameHost* rfh) {
  if (!rfh)
    return std::make_pair(-1, -1);

  return std::make_pair(rfh->GetProcess()->GetID(), rfh->GetRoutingID());
}

int GetRequestID(const base::DictionaryValue& options) {
  int request_id = -1;
  options.GetInteger(kPreviewRequestID, &request_id);
  return request_id;
}

}  // namespace

PrintPreviewMessageHandler::PrintPreviewMessageHandler(
    WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      weak_ptr_factory_(this) {
  DCHECK(web_contents);
}

PrintPreviewMessageHandler::~PrintPreviewMessageHandler() {}

void PrintPreviewMessageHandler::OnMetafileReadyForPrinting(
    content::RenderFrameHost* rfh,
    const PrintHostMsg_DidPreviewDocument_Params& params) {
  // Always try to stop the worker.
  StopWorker(params.document_cookie);

  if (params.expected_pages_count <= 0) {
    NOTREACHED();
    return;
  }

  auto key = GetKey(rfh);
  if (base::ContainsKey(print_to_pdf_options_map_, key))
    print_to_pdf_options_map_.erase(key);

  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&CopyPDFDataOnIOThread, params),
      base::Bind(&PrintPreviewMessageHandler::RunPrintToPDFCallback,
                 weak_ptr_factory_.GetWeakPtr(),
                 params.preview_request_id,
                 params.data_size));
}

void PrintPreviewMessageHandler::OnError(content::RenderFrameHost* rfh,
                                          int document_cookie,
                                          const std::string& message) {
  StopWorker(document_cookie);

  auto key = GetKey(rfh);
  if (base::ContainsKey(print_to_pdf_options_map_, key)) {
    auto options = print_to_pdf_options_map_[key].get();
    RunPrintToPDFCallbackWithMessage(
        GetRequestID(*options), message, 0, nullptr);
    print_to_pdf_options_map_.erase(key);
  }
}

void PrintPreviewMessageHandler::OnPrintPreviewFailed(
    content::RenderFrameHost* rfh,
    int document_cookie) {
  StopWorker(document_cookie);

  OnError(rfh, document_cookie, "Failed");
}

void PrintPreviewMessageHandler::OnPrintPreviewCancelled(
    content::RenderFrameHost* rfh,
    int document_cookie) {
  StopWorker(document_cookie);

  OnError(rfh, document_cookie, "Cancelled");
}

void PrintPreviewMessageHandler::OnPrintPreviewInvalidPrinterSettings(
    content::RenderFrameHost* rfh,
    int document_cookie) {
  StopWorker(document_cookie);

  OnError(rfh, document_cookie, "Invalid Settings");
}

bool PrintPreviewMessageHandler::OnMessageReceived(
    const IPC::Message& message,
    content::RenderFrameHost* render_frame_host) {

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(PrintPreviewMessageHandler, message,
                                   render_frame_host)
    IPC_MESSAGE_HANDLER(PrintHostMsg_RequestPrintPreview,
                        OnRequestPrintPreview)
    IPC_MESSAGE_HANDLER(PrintHostMsg_MetafileReadyForPrinting,
                        OnMetafileReadyForPrinting)
    IPC_MESSAGE_HANDLER(PrintHostMsg_PrintPreviewFailed,
                        OnPrintPreviewFailed)
    IPC_MESSAGE_HANDLER(PrintHostMsg_PrintPreviewCancelled,
                        OnPrintPreviewCancelled)
    IPC_MESSAGE_HANDLER(PrintHostMsg_PrintPreviewInvalidPrinterSettings,
                        OnPrintPreviewInvalidPrinterSettings)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void PrintPreviewMessageHandler::OnRequestPrintPreview(
    content::RenderFrameHost* rfh,
    const PrintHostMsg_RequestPrintPreview_Params& params) {
  if (params.webnode_only) {
    PrintViewManager::FromWebContents(
        web_contents())->PrintPreviewForWebNode(rfh);
  }

  auto key = GetKey(rfh);
  if (base::ContainsKey(print_to_pdf_options_map_, key)) {
    auto options = print_to_pdf_options_map_[key].get();
    options->SetBoolean(printing::kSettingPrintToPDF, true);
    options->SetInteger(kSettingDpiHorizontal, kPointsPerInch);
    options->SetInteger(kSettingDpiVertical, kPointsPerInch);

    rfh->Send(new PrintMsg_PrintPreview(rfh->GetRoutingID(), *options));
  }
}

void PrintPreviewMessageHandler::PrintToPDF(
    const base::DictionaryValue& options,
    const atom::api::WebContents::PrintToPDFCallback& callback) {
  int request_id = GetRequestID(options);
  print_to_pdf_callback_map_[request_id] = callback;

  content::RenderFrameHost* rfh = printing::GetFrameToPrint(web_contents());
  if (rfh) {
    auto key = GetKey(rfh);
    if (base::ContainsKey(print_to_pdf_options_map_, key)) {
      RunPrintToPDFCallbackWithMessage(request_id, "Busy", 0, nullptr);
      return;
    }

    print_to_pdf_options_map_[key] = options.CreateDeepCopy();
  } else {
    RunPrintToPDFCallbackWithMessage(request_id, "Aborted", 0, nullptr);
    return;
  }

  printing::StartPrint(web_contents(), false, false);
}

void PrintPreviewMessageHandler::RunPrintToPDFCallback(
    int request_id, uint32_t data_size, char* data) {
  RunPrintToPDFCallbackWithMessage(request_id, "", data_size, data);
}

void PrintPreviewMessageHandler::RunPrintToPDFCallbackWithMessage(
    int request_id, const std::string& message,
    uint32_t data_size, char* data) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  if (data) {
    v8::Local<v8::Value> buffer = node::Buffer::New(isolate,
        data, static_cast<size_t>(data_size)).ToLocalChecked();
    print_to_pdf_callback_map_[request_id].Run(v8::Null(isolate), buffer);
  } else {
    v8::Local<v8::String> error_message = v8::String::NewFromUtf8(isolate,
        (message.empty() ? "Failed" : message).c_str());
    print_to_pdf_callback_map_[request_id].Run(
        v8::Exception::Error(error_message), v8::Null(isolate));
  }
  print_to_pdf_callback_map_.erase(request_id);

  auto manager = PrintViewManager::FromWebContents(web_contents());
  if (manager) {
    manager->PrintPreviewDone();
  }
}

}  // namespace printing
