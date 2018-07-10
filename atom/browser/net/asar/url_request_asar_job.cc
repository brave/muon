// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/asar/url_request_asar_job.h"

#include <string>
#include <utility>
#include <vector>

#include "atom/common/asar/archive.h"
#include "atom/common/asar/asar_util.h"
#include "atom/common/atom_constants.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "base/synchronization/lock.h"
#include "base/task_runner.h"
#include "net/base/file_stream.h"
#include "net/base/filename_util.h"
#include "net/base/io_buffer.h"
#include "net/base/load_flags.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/filter/gzip_source_stream.h"
#include "net/filter/source_stream.h"
#include "net/http/http_util.h"
#include "net/url_request/url_request_status.h"

#if defined(OS_WIN)
#include "base/win/shortcut.h"
#endif

namespace asar {

namespace {

void Initialize(
    const base::FilePath& full_path,
    std::shared_ptr<Archive>& archive,  // NOLINT
    base::FilePath* file_path,
    Archive::FileInfo* file_info,
    URLRequestAsarJob::JobType* type) {
  // Determine whether it is an asar file.
  base::FilePath asar_path, relative_path;
  if (!GetAsarArchivePath(full_path, &asar_path, &relative_path)) {
    *file_path = full_path;
    *type = URLRequestAsarJob::TYPE_FILE;
    return;
  }

  archive =
      GetOrCreateAsarArchive(asar_path);
  if (!archive || !archive->GetFileInfo(relative_path, file_info)) {
    *type = URLRequestAsarJob::TYPE_ERROR;
    return;
  }

  if (file_info->unpacked) {
    archive->CopyFileOut(relative_path, file_path);
    *type = URLRequestAsarJob::TYPE_FILE;
    return;
  }

  *file_path = relative_path;
  *type = URLRequestAsarJob::TYPE_ASAR;
}

}  // namespace

URLRequestAsarJob::FileMetaInfo::FileMetaInfo()
    : file_size(0),
      mime_type_result(false),
      file_exists(false),
      is_directory(false) {
}

URLRequestAsarJob::URLRequestAsarJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const scoped_refptr<base::TaskRunner> file_task_runner)
    : net::URLRequestJob(request, network_delegate),
      type_(TYPE_ERROR),
      remaining_bytes_(0),
      seek_offset_(0),
      range_parse_result_(net::OK),
      file_task_runner_(file_task_runner),
      weak_ptr_factory_(this) {
  net::FileURLToFilePath(request->url(), &full_path_);
}

URLRequestAsarJob::~URLRequestAsarJob() {}

void URLRequestAsarJob::InitializeAsarJob() {
  stream_.reset(new net::FileStream(file_task_runner_));
}

void URLRequestAsarJob::InitializeFileJob() {
  stream_.reset(new net::FileStream(file_task_runner_));
}

void URLRequestAsarJob::Start() {
  file_task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&Initialize,
          full_path_, std::ref(archive_), &file_path_, &file_info_, &type_),
      base::Bind(&URLRequestAsarJob::DidInitialize,
          weak_ptr_factory_.GetWeakPtr()));
}

void URLRequestAsarJob::DidInitialize() {
  if (type_ == TYPE_ASAR) {
    InitializeAsarJob();
    int flags = base::File::FLAG_OPEN |
                base::File::FLAG_READ |
                base::File::FLAG_ASYNC;
    int rv = stream_->Open(archive_->path(), flags,
                           base::Bind(&URLRequestAsarJob::DidOpen,
                                      weak_ptr_factory_.GetWeakPtr()));
    if (rv != net::ERR_IO_PENDING)
      DidOpen(rv);
  } else if (type_ == TYPE_FILE) {
    InitializeFileJob();
    auto* meta_info = new FileMetaInfo();
    file_task_runner_->PostTaskAndReply(
        FROM_HERE,
        base::Bind(&URLRequestAsarJob::FetchMetaInfo, file_path_,
                   base::Unretained(meta_info)),
        base::Bind(&URLRequestAsarJob::DidFetchMetaInfo,
                   weak_ptr_factory_.GetWeakPtr(),
                   base::Owned(meta_info)));
  } else {
    NotifyStartError(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                           net::ERR_FILE_NOT_FOUND));
  }
}

void URLRequestAsarJob::Kill() {
  stream_.reset();
  weak_ptr_factory_.InvalidateWeakPtrs();

  URLRequestJob::Kill();
}

int URLRequestAsarJob::ReadRawData(net::IOBuffer* dest, int dest_size) {
  if (remaining_bytes_ < dest_size)
    dest_size = static_cast<int>(remaining_bytes_);

  // If we should copy zero bytes because |remaining_bytes_| is zero, short
  // circuit here.
  if (!dest_size)
    return 0;

  int rv = stream_->Read(dest,
                         dest_size,
                         base::Bind(&URLRequestAsarJob::DidRead,
                                    weak_ptr_factory_.GetWeakPtr(),
                                    base::RetainedRef(dest)));
  if (rv >= 0) {
    remaining_bytes_ -= rv;
    DCHECK_GE(remaining_bytes_, 0);
  }

  return rv;
}

bool URLRequestAsarJob::IsRedirectResponse(GURL* location,
                                           int* http_status_code,
                                           bool* insecure_scheme_was_upgraded) {
  if (type_ != TYPE_FILE)
    return false;
#if defined(OS_WIN)
  // Follow a Windows shortcut.
  // We just resolve .lnk file, ignore others.
  if (!base::LowerCaseEqualsASCII(file_path_.Extension(), ".lnk"))
    return false;

  base::FilePath new_path = file_path_;
  bool resolved;
  resolved = base::win::ResolveShortcut(new_path, &new_path, NULL);

  // If shortcut is not resolved succesfully, do not redirect.
  if (!resolved)
    return false;

  *location = net::FilePathToFileURL(new_path);
  *http_status_code = 301;
  return true;
#else
  return false;
#endif
}

std::unique_ptr<net::SourceStream> URLRequestAsarJob::SetUpSourceStream() {
  std::unique_ptr<net::SourceStream> source =
    URLRequestJob::SetUpSourceStream();
  if (!base::LowerCaseEqualsASCII(file_path_.Extension(), ".svgz"))
    return source;

  return net::GzipSourceStream::Create(std::move(source),
                                       net::SourceStream::TYPE_GZIP);
}

bool URLRequestAsarJob::GetMimeType(std::string* mime_type) const {
  if (type_ == TYPE_ASAR) {
    return net::GetMimeTypeFromFile(file_path_, mime_type);
  } else {
    if (meta_info_.mime_type_result) {
      *mime_type = meta_info_.mime_type;
      return true;
    }
    return false;
  }
}

void URLRequestAsarJob::SetExtraRequestHeaders(
    const net::HttpRequestHeaders& headers) {
  std::string range_header;
  if (headers.GetHeader(net::HttpRequestHeaders::kRange, &range_header)) {
    // This job only cares about the Range header. This method stashes the value
    // for later use in DidOpen(), which is responsible for some of the range
    // validation as well. NotifyStartError is not legal to call here since
    // the job has not started.
    std::vector<net::HttpByteRange> ranges;
    if (net::HttpUtil::ParseRangeHeader(range_header, &ranges)) {
      if (ranges.size() == 1) {
        byte_range_ = ranges[0];
      } else {
        range_parse_result_ = net::ERR_REQUEST_RANGE_NOT_SATISFIABLE;
      }
    }
  }
}

int URLRequestAsarJob::GetResponseCode() const {
  // Request Job gets created only if path exists.
  return 200;
}

void URLRequestAsarJob::GetResponseInfo(net::HttpResponseInfo* info) {
  std::string status("HTTP/1.1 200 OK");
  auto* headers = new net::HttpResponseHeaders(status);

  headers->AddHeader(atom::kCORSHeader);
  info->headers = headers;
}

void URLRequestAsarJob::FetchMetaInfo(const base::FilePath& file_path,
                                      FileMetaInfo* meta_info) {
  base::File::Info file_info;
  meta_info->file_exists = base::GetFileInfo(file_path, &file_info);
  if (meta_info->file_exists) {
    meta_info->file_size = file_info.size;
    meta_info->is_directory = file_info.is_directory;
  }
  // On Windows GetMimeTypeFromFile() goes to the registry. Thus it should be
  // done in WorkerPool.
  meta_info->mime_type_result =
      net::GetMimeTypeFromFile(file_path, &meta_info->mime_type);
}

void URLRequestAsarJob::DidFetchMetaInfo(const FileMetaInfo* meta_info) {
  meta_info_ = *meta_info;
  if (!meta_info_.file_exists || meta_info_.is_directory) {
    DidOpen(net::ERR_FILE_NOT_FOUND);
    return;
  }

  int flags = base::File::FLAG_OPEN |
              base::File::FLAG_READ |
              base::File::FLAG_ASYNC;
  int rv = stream_->Open(file_path_, flags,
                         base::Bind(&URLRequestAsarJob::DidOpen,
                                    weak_ptr_factory_.GetWeakPtr()));
  if (rv != net::ERR_IO_PENDING)
    DidOpen(rv);
}

void URLRequestAsarJob::DidOpen(int result) {
  if (result != net::OK) {
    NotifyStartError(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                           result));
    return;
  }

  if (range_parse_result_ != net::OK) {
    NotifyStartError(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                           range_parse_result_));
    return;
  }

  int64_t file_size, read_offset;
  if (type_ == TYPE_ASAR) {
    file_size = file_info_.size;
    read_offset = file_info_.offset;
  } else {
    file_size = meta_info_.file_size;
    read_offset = 0;
  }

  if (!byte_range_.ComputeBounds(file_size)) {
    NotifyStartError(
        net::URLRequestStatus(net::URLRequestStatus::FAILED,
                              net::ERR_REQUEST_RANGE_NOT_SATISFIABLE));
    return;
  }

  remaining_bytes_ = byte_range_.last_byte_position() -
                     byte_range_.first_byte_position() + 1;
  seek_offset_ = byte_range_.first_byte_position() + read_offset;

  if (remaining_bytes_ > 0 && seek_offset_ != 0) {
    int rv = stream_->Seek(seek_offset_,
                           base::Bind(&URLRequestAsarJob::DidSeek,
                                      weak_ptr_factory_.GetWeakPtr()));
    if (rv != net::ERR_IO_PENDING) {
      // stream_->Seek() failed, so pass an intentionally erroneous value
      // into DidSeek().
      DidSeek(-1);
    }
  } else {
    // We didn't need to call stream_->Seek() at all, so we pass to DidSeek()
    // the value that would mean seek success. This way we skip the code
    // handling seek failure.
    DidSeek(seek_offset_);
  }
}

void URLRequestAsarJob::DidSeek(int64_t result) {
  if (result != seek_offset_) {
    NotifyStartError(
        net::URLRequestStatus(net::URLRequestStatus::FAILED,
                              net::ERR_REQUEST_RANGE_NOT_SATISFIABLE));
    return;
  }
  set_expected_content_size(remaining_bytes_);
  NotifyHeadersComplete();
}

void URLRequestAsarJob::DidRead(scoped_refptr<net::IOBuffer> buf, int result) {
  if (result >= 0) {
    remaining_bytes_ -= result;
    DCHECK_GE(remaining_bytes_, 0);
  }

  buf = nullptr;

  ReadRawDataComplete(result);
}

}  // namespace asar
