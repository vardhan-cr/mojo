// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_FILES_FILE_IMPL_H_
#define SERVICES_FILES_FILE_IMPL_H_

#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/files/directory.mojom.h"

namespace mojo {
namespace files {

class FileImpl : public File {
 public:
  // TODO(vtl): Will need more for, e.g., |Reopen()|.
  FileImpl(InterfaceRequest<File> request, base::ScopedFD file_fd);
  ~FileImpl() override;

  // |File| implementation:
  void Close(const Callback<void(Error)>& callback) override;
  void Read(uint32_t num_bytes_to_read,
            int64_t offset,
            Whence whence,
            const Callback<void(Error, Array<uint8_t>)>& callback) override;
  void Write(Array<uint8_t> bytes_to_write,
             int64_t offset,
             Whence whence,
             const Callback<void(Error, uint32_t)>& callback) override;
  void ReadToStream(ScopedDataPipeProducerHandle source,
                    int64_t offset,
                    Whence whence,
                    int64_t num_bytes_to_read,
                    const Callback<void(Error)>& callback) override;
  void WriteFromStream(ScopedDataPipeConsumerHandle sink,
                       int64_t offset,
                       Whence whence,
                       const Callback<void(Error)>& callback) override;
  void Tell(const Callback<void(Error, int64_t)>& callback) override;
  void Seek(int64_t offset,
            Whence whence,
            const Callback<void(Error, int64_t)>& callback) override;
  void Stat(const Callback<void(Error, FileInformationPtr)>& callback) override;
  void Truncate(int64_t size, const Callback<void(Error)>& callback) override;
  void Touch(TimespecOrNowPtr atime,
             TimespecOrNowPtr mtime,
             const Callback<void(Error)>& callback) override;
  void Dup(InterfaceRequest<File> file,
           const Callback<void(Error)>& callback) override;
  void Reopen(InterfaceRequest<File> file,
              uint32_t open_flags,
              const Callback<void(Error)>& callback) override;
  void AsBuffer(
      const Callback<void(Error, ScopedSharedBufferHandle)>& callback) override;

 private:
  StrongBinding<File> binding_;
  base::ScopedFD file_fd_;

  DISALLOW_COPY_AND_ASSIGN(FileImpl);
};

}  // namespace files
}  // namespace mojo

#endif  // SERVICES_FILES_FILE_IMPL_H_
