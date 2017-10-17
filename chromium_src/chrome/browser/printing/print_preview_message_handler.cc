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
#include "chrome/browser/printing/printer_query.h"
#include "components/printing/common/print_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "printing/page_size_margins.h"
#include "printing/pdf_metafile_skia.h"
#include "printing/print_job_constants.h"

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

}  // namespace

PrintPreviewMessageHandler::PrintPreviewMessageHandler(
    WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      weak_ptr_factory_(this) {
  DCHECK(web_contents);
}

PrintPreviewMessageHandler::~PrintPreviewMessageHandler() {}

void PrintPreviewMessageHandler::OnMetafileReadyForPrinting(
    const PrintHostMsg_DidPreviewDocument_Params& params) {
  // Always try to stop the worker.
  StopWorker(params.document_cookie);

  if (params.expected_pages_count <= 0) {
    NOTREACHED();
    return;
  }

  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&CopyPDFDataOnIOThread, params),
      base::Bind(&PrintPreviewMessageHandler::RunPrintToPDFCallback,
                 weak_ptr_factory_.GetWeakPtr(),
                 params.preview_request_id,
                 params.data_size));
}

void PrintPreviewMessageHandler::OnPrintPreviewFailed(int document_cookie) {
  StopWorker(document_cookie);
}

bool PrintPreviewMessageHandler::OnMessageReceived(
    const IPC::Message& message,
    content::RenderFrameHost* render_frame_host) {

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(PrintPreviewMessageHandler, message,
                                   render_frame_host)
    IPC_MESSAGE_HANDLER(PrintHostMsg_RequestPrintPreview,
                        OnRequestPrintPreview)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  if (handled)
    return true;

  IPC_BEGIN_MESSAGE_MAP(PrintPreviewMessageHandler, message)
    IPC_MESSAGE_HANDLER(PrintHostMsg_MetafileReadyForPrinting,
                        OnMetafileReadyForPrinting)
    IPC_MESSAGE_HANDLER(PrintHostMsg_PrintPreviewFailed,
                        OnPrintPreviewFailed)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void PrintPreviewMessageHandler::OnRequestPrintPreview(
    content::RenderFrameHost* render_frame_host,
    const PrintHostMsg_RequestPrintPreview_Params& params) {
  if (params.webnode_only) {
    PrintViewManager::FromWebContents(web_contents())->PrintPreviewForWebNode(
        render_frame_host);
  }
}

void PrintPreviewMessageHandler::PrintToPDF(
    const base::DictionaryValue& options,
    const atom::api::WebContents::PrintToPDFCallback& callback) {
  PrintViewManager::FromWebContents(web_contents())->PrintPreviewNow(
        web_contents()->GetMainFrame(), false);

  int request_id = -1;
  options.GetInteger(kPreviewRequestID, &request_id);
  print_to_pdf_callback_map_[request_id] = callback;

  WebContents* initiator = web_contents();
  content::RenderFrameHost* rfh =
      initiator
          ? PrintViewManager::FromWebContents(initiator)->print_preview_rfh()
          : nullptr;

  if (rfh) {
    std::unique_ptr<base::DictionaryValue> new_options(options.DeepCopy());
    new_options->SetBoolean(printing::kSettingPrintToPDF, true);
    new_options->SetInteger(kPreviewInitiatorHostId,
                        rfh->GetProcess()->GetID());
    new_options->SetInteger(kPreviewInitiatorRoutingId,
                        rfh->GetRoutingID());
    rfh->Send(new PrintMsg_PrintPreview(rfh->GetRoutingID(), *new_options));
  }
}

void PrintPreviewMessageHandler::RunPrintToPDFCallback(
    int request_id, uint32_t data_size, char* data) {
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
        "Fail to generate PDF");
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
