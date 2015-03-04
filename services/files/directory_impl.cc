// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/files/directory_impl.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/posix/eintr_wrapper.h"
#include "build/build_config.h"
#include "services/files/file_impl.h"
#include "services/files/util.h"

namespace mojo {
namespace files {

namespace {

Error ValidateOpenFlags(uint32_t open_flags, bool is_directory) {
  // Treat unknown flags as "unimplemented".
  if ((open_flags &
       ~(kOpenFlagRead | kOpenFlagWrite | kOpenFlagCreate | kOpenFlagExclusive |
         kOpenFlagAppend | kOpenFlagTruncate)))
    return ERROR_UNIMPLEMENTED;

  // At least one of |kOpenFlagRead| or |kOpenFlagWrite| must be set.
  if (!(open_flags & (kOpenFlagRead | kOpenFlagWrite)))
    return ERROR_INVALID_ARGUMENT;

  // |kOpenFlagCreate| requires |kOpenFlagWrite|.
  if ((open_flags & kOpenFlagCreate) && !(open_flags & kOpenFlagWrite))
    return ERROR_INVALID_ARGUMENT;

  // |kOpenFlagExclusive| requires |kOpenFlagCreate|.
  if ((open_flags & kOpenFlagExclusive) && !(open_flags & kOpenFlagCreate))
    return ERROR_INVALID_ARGUMENT;

  if (is_directory) {
    // Check that file-only flags aren't set.
    if ((open_flags & (kOpenFlagAppend | kOpenFlagTruncate)))
      return ERROR_INVALID_ARGUMENT;
    return ERROR_OK;
  }

  // File-only flags:

  // |kOpenFlagAppend| requires |kOpenFlagWrite|.
  if ((open_flags & kOpenFlagAppend) && !(open_flags & kOpenFlagWrite))
    return ERROR_INVALID_ARGUMENT;

  // |kOpenFlagTruncate| requires |kOpenFlagWrite|.
  if ((open_flags & kOpenFlagTruncate) && !(open_flags & kOpenFlagWrite))
    return ERROR_INVALID_ARGUMENT;

  return ERROR_OK;
}

}  // namespace

DirectoryImpl::DirectoryImpl(InterfaceRequest<Directory> request,
                             base::ScopedFD dir_fd,
                             scoped_ptr<base::ScopedTempDir> temp_dir)
    : binding_(this, request.Pass()),
      dir_fd_(dir_fd.Pass()),
      temp_dir_(temp_dir.Pass()) {
  DCHECK(dir_fd_.is_valid());
}

DirectoryImpl::~DirectoryImpl() {
}

void DirectoryImpl::Read(
    const Callback<void(Error, Array<DirectoryEntryPtr>)>& callback) {
  // TODO(vtl): FIXME sooner
  NOTIMPLEMENTED();
  callback.Run(ERROR_UNIMPLEMENTED, Array<DirectoryEntryPtr>());
}

void DirectoryImpl::Stat(
    const Callback<void(Error, FileInformationPtr)>& callback) {
  // TODO(vtl): FIXME sooner
  NOTIMPLEMENTED();
  callback.Run(ERROR_UNIMPLEMENTED, nullptr);
}

void DirectoryImpl::Touch(TimespecOrNowPtr atime,
                          TimespecOrNowPtr mtime,
                          const Callback<void(Error)>& callback) {
  // TODO(vtl): FIXME sooner
  NOTIMPLEMENTED();
  callback.Run(ERROR_UNIMPLEMENTED);
}

// TODO(vtl): Move the implementation to a thread pool.
void DirectoryImpl::OpenFile(const String& path,
                             InterfaceRequest<File> file,
                             uint32_t open_flags,
                             const Callback<void(Error)>& callback) {
  DCHECK(!path.is_null());
  DCHECK(dir_fd_.is_valid());

  if (Error error = IsPathValid(path)) {
    callback.Run(error);
    return;
  }
  // TODO(vtl): Make sure the path doesn't exit this directory (if appropriate).
  // TODO(vtl): Maybe allow absolute paths?

  if (Error error = ValidateOpenFlags(open_flags, false)) {
    callback.Run(error);
    return;
  }

  int flags = 0;
  if ((open_flags & kOpenFlagRead))
    flags |= (open_flags & kOpenFlagWrite) ? O_RDWR : O_RDONLY;
  else
    flags |= O_WRONLY;
  if ((open_flags & kOpenFlagCreate))
    flags |= O_CREAT;
  if ((open_flags & kOpenFlagExclusive))
    flags |= O_EXCL;
  if ((open_flags & kOpenFlagAppend))
    flags |= O_APPEND;
  if ((open_flags & kOpenFlagTruncate))
    flags |= O_TRUNC;

  base::ScopedFD file_fd(
      HANDLE_EINTR(openat(dir_fd_.get(), path.get().c_str(), flags, 0600)));
  if (!file_fd.is_valid()) {
    callback.Run(ErrnoToError(errno));
    return;
  }

  if (file.is_pending())
    new FileImpl(file.Pass(), file_fd.Pass());
  callback.Run(ERROR_OK);
}

void DirectoryImpl::OpenDirectory(const String& path,
                                  InterfaceRequest<Directory> directory,
                                  uint32_t open_flags,
                                  const Callback<void(Error)>& callback) {
  // TODO(vtl): FIXME sooner
  NOTIMPLEMENTED();
  callback.Run(ERROR_UNIMPLEMENTED);
}

void DirectoryImpl::Rename(const String& path,
                           const String& new_path,
                           const Callback<void(Error)>& callback) {
  DCHECK(!path.is_null());
  DCHECK(!new_path.is_null());
  DCHECK(dir_fd_.is_valid());

  if (Error error = IsPathValid(path)) {
    callback.Run(error);
    return;
  }
  if (Error error = IsPathValid(new_path)) {
    callback.Run(error);
    return;
  }
  // TODO(vtl): see TODOs about |path| in OpenFile()

  if (renameat(dir_fd_.get(), path.get().c_str(), dir_fd_.get(),
               new_path.get().c_str())) {
    callback.Run(ErrnoToError(errno));
    return;
  }

  callback.Run(ERROR_OK);
}

void DirectoryImpl::Delete(const String& path,
                           uint32_t delete_flags,
                           const Callback<void(Error)>& callback) {
  // TODO(vtl): FIXME sooner
  NOTIMPLEMENTED();
  callback.Run(ERROR_UNIMPLEMENTED);
}

}  // namespace files
}  // namespace mojo
