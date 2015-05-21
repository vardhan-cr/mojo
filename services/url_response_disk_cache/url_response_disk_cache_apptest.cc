// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_util.h"
#include "base/rand_util.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/services/network/public/interfaces/url_loader.mojom.h"
#include "mojo/services/url_response_disk_cache/public/interfaces/url_response_disk_cache.mojom.h"
#include "services/url_response_disk_cache/kTestData.h"

namespace mojo {

namespace {

class URLResponseDiskCacheAppTest : public mojo::test::ApplicationTestBase {
 public:
  URLResponseDiskCacheAppTest() : ApplicationTestBase() {}
  ~URLResponseDiskCacheAppTest() override {}

  void SetUp() override {
    mojo::test::ApplicationTestBase::SetUp();
    application_impl()->ConnectToService("mojo:url_response_disk_cache",
                                         &url_response_disk_cache_);
  }

 protected:
  URLResponseDiskCachePtr url_response_disk_cache_;

  DISALLOW_COPY_AND_ASSIGN(URLResponseDiskCacheAppTest);
};

base::FilePath toPath(Array<uint8_t> path) {
  if (path.is_null()) {
    return base::FilePath();
  }
  return base::FilePath(
      std::string(reinterpret_cast<char*>(&path.front()), path.size()));
}

}  // namespace

TEST_F(URLResponseDiskCacheAppTest, GetFile) {
  URLResponsePtr url_response = mojo::URLResponse::New();
  url_response->url = "http://www.example.com/1";
  url_response->headers = Array<String>(1);
  url_response->headers[0] = base::StringPrintf("ETag: %f", base::RandDouble());
  DataPipe pipe;
  std::string content = base::RandBytesAsString(32);
  uint32_t num_bytes = content.size();
  ASSERT_EQ(MOJO_RESULT_OK,
            WriteDataRaw(pipe.producer_handle.get(), content.c_str(),
                         &num_bytes, MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));
  ASSERT_EQ(content.size(), num_bytes);
  pipe.producer_handle.reset();
  url_response->body = pipe.consumer_handle.Pass();
  base::FilePath file;
  base::FilePath cache_dir;
  url_response_disk_cache_->GetFile(
      url_response.Pass(),
      [&file, &cache_dir](Array<uint8_t> received_file_path,
                          Array<uint8_t> received_cache_dir_path) {
        file = toPath(received_file_path.Pass());
        cache_dir = toPath(received_cache_dir_path.Pass());
      });
  url_response_disk_cache_.WaitForIncomingResponse();
  ASSERT_FALSE(file.empty());
  std::string received_content;
  ASSERT_TRUE(base::ReadFileToString(file, &received_content));
  EXPECT_EQ(content, received_content);
}

TEST_F(URLResponseDiskCacheAppTest, GetExtractedContent) {
  URLResponsePtr url_response = mojo::URLResponse::New();
  url_response->url = "http://www.example.com/2";
  url_response->headers = Array<String>(1);
  url_response->headers[0] = base::StringPrintf("ETag: %f", base::RandDouble());
  DataPipe pipe;
  std::string content = base::RandBytesAsString(32);
  uint32_t num_bytes = kTestData.size;
  ASSERT_EQ(MOJO_RESULT_OK,
            WriteDataRaw(pipe.producer_handle.get(), kTestData.data, &num_bytes,
                         MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));
  ASSERT_EQ(kTestData.size, num_bytes);
  pipe.producer_handle.reset();
  url_response->body = pipe.consumer_handle.Pass();
  base::FilePath extracted_dir;
  base::FilePath cache_dir;
  url_response_disk_cache_->GetExtractedContent(
      url_response.Pass(),
      [&extracted_dir, &cache_dir](Array<uint8_t> received_extracted_dir,
                                   Array<uint8_t> received_cache_dir_path) {
        extracted_dir = toPath(received_extracted_dir.Pass());
        cache_dir = toPath(received_cache_dir_path.Pass());
      });
  url_response_disk_cache_.WaitForIncomingResponse();
  ASSERT_FALSE(extracted_dir.empty());
  base::FilePath file1 = extracted_dir.Append("file1");
  std::string file_content;
  ASSERT_TRUE(base::ReadFileToString(file1, &file_content));
  EXPECT_EQ("hello\n", file_content);
  base::FilePath file2 = extracted_dir.Append("file2");
  ASSERT_TRUE(base::ReadFileToString(file2, &file_content));
  EXPECT_EQ("world\n", file_content);
}

TEST_F(URLResponseDiskCacheAppTest, CacheTest) {
  URLResponsePtr url_response = mojo::URLResponse::New();
  url_response->url = "http://www.example.com/3";
  url_response->headers = Array<String>(1);
  std::string etag = base::StringPrintf("ETag: %f", base::RandDouble());
  url_response->headers[0] = etag;
  DataPipe pipe1;
  std::string content = base::RandBytesAsString(32);
  uint32_t num_bytes = content.size();
  ASSERT_EQ(MOJO_RESULT_OK,
            WriteDataRaw(pipe1.producer_handle.get(), content.c_str(),
                         &num_bytes, MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));
  ASSERT_EQ(content.size(), num_bytes);
  pipe1.producer_handle.reset();
  url_response->body = pipe1.consumer_handle.Pass();
  base::FilePath file;
  base::FilePath cache_dir;
  url_response_disk_cache_->GetFile(
      url_response.Pass(),
      [&file, &cache_dir](Array<uint8_t> received_file_path,
                          Array<uint8_t> received_cache_dir_path) {
        file = toPath(received_file_path.Pass());
        cache_dir = toPath(received_cache_dir_path.Pass());
      });
  url_response_disk_cache_.WaitForIncomingResponse();
  ASSERT_FALSE(file.empty());
  std::string received_content;
  ASSERT_TRUE(base::ReadFileToString(file, &received_content));
  EXPECT_EQ(content, received_content);
  base::FilePath saved_file = cache_dir.Append("file");
  EXPECT_FALSE(base::PathExists(saved_file));
  std::string cached_content = base::RandBytesAsString(32);
  ASSERT_TRUE(base::WriteFile(saved_file, cached_content.data(),
                              cached_content.size()));

  // Request using a response for the same URL, with the same etag, but a
  // different content. The cached value should be returned.
  url_response = mojo::URLResponse::New();
  url_response->url = "http://www.example.com/3";
  url_response->headers = Array<String>(1);
  url_response->headers[0] = etag;
  DataPipe pipe2;
  std::string new_content = base::RandBytesAsString(32);
  num_bytes = new_content.size();
  ASSERT_EQ(MOJO_RESULT_OK,
            WriteDataRaw(pipe2.producer_handle.get(), new_content.c_str(),
                         &num_bytes, MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));
  ASSERT_EQ(new_content.size(), num_bytes);
  pipe2.producer_handle.reset();
  url_response->body = pipe2.consumer_handle.Pass();
  url_response_disk_cache_->GetFile(
      url_response.Pass(),
      [&file, &cache_dir](Array<uint8_t> received_file_path,
                          Array<uint8_t> received_cache_dir_path) {
        file = toPath(received_file_path.Pass());
        cache_dir = toPath(received_cache_dir_path.Pass());
      });
  url_response_disk_cache_.WaitForIncomingResponse();
  ASSERT_FALSE(file.empty());
  ASSERT_TRUE(base::ReadFileToString(file, &received_content));
  EXPECT_EQ(content, received_content);
  saved_file = cache_dir.Append("file");
  EXPECT_TRUE(base::PathExists(saved_file));
  std::string received_cached_content;
  ASSERT_TRUE(base::ReadFileToString(saved_file, &received_cached_content));
  EXPECT_EQ(cached_content, received_cached_content);

  // Request using a response for the same URL, with the a different etag. Check
  // that the new content is returned, and the cached files is deleted.
  url_response = mojo::URLResponse::New();
  url_response->url = "http://www.example.com/3";
  url_response->headers = Array<String>(1);
  url_response->headers[0] = base::StringPrintf("ETag: %f", base::RandDouble());
  DataPipe pipe3;
  new_content = base::RandBytesAsString(32);
  num_bytes = new_content.size();
  ASSERT_EQ(MOJO_RESULT_OK,
            WriteDataRaw(pipe3.producer_handle.get(), new_content.c_str(),
                         &num_bytes, MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));
  ASSERT_EQ(new_content.size(), num_bytes);
  pipe3.producer_handle.reset();
  url_response->body = pipe3.consumer_handle.Pass();
  url_response_disk_cache_->GetFile(
      url_response.Pass(),
      [&file, &cache_dir](Array<uint8_t> received_file_path,
                          Array<uint8_t> received_cache_dir_path) {
        file = toPath(received_file_path.Pass());
        cache_dir = toPath(received_cache_dir_path.Pass());
      });
  url_response_disk_cache_.WaitForIncomingResponse();
  ASSERT_FALSE(file.empty());
  ASSERT_TRUE(base::ReadFileToString(file, &received_content));
  EXPECT_EQ(new_content, received_content);
  saved_file = cache_dir.Append("file");
  EXPECT_FALSE(base::PathExists(saved_file));
  ASSERT_TRUE(base::WriteFile(saved_file, cached_content.data(),
                              cached_content.size()));

  // Request using a response without an etag header. Check that the new
  // content is returned, and the cached files is deleted.
  url_response = mojo::URLResponse::New();
  url_response->url = "http://www.example.com/3";
  DataPipe pipe4;
  new_content = base::RandBytesAsString(32);
  num_bytes = new_content.size();
  ASSERT_EQ(MOJO_RESULT_OK,
            WriteDataRaw(pipe4.producer_handle.get(), new_content.c_str(),
                         &num_bytes, MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));
  ASSERT_EQ(new_content.size(), num_bytes);
  pipe4.producer_handle.reset();
  url_response->body = pipe4.consumer_handle.Pass();
  url_response_disk_cache_->GetFile(
      url_response.Pass(),
      [&file, &cache_dir](Array<uint8_t> received_file_path,
                          Array<uint8_t> received_cache_dir_path) {
        file = toPath(received_file_path.Pass());
        cache_dir = toPath(received_cache_dir_path.Pass());
      });
  url_response_disk_cache_.WaitForIncomingResponse();
  ASSERT_FALSE(file.empty());
  ASSERT_TRUE(base::ReadFileToString(file, &received_content));
  EXPECT_EQ(new_content, received_content);
  saved_file = cache_dir.Append("file");
  EXPECT_FALSE(base::PathExists(saved_file));
  ASSERT_TRUE(base::WriteFile(saved_file, cached_content.data(),
                              cached_content.size()));

  // Request using a response with an etag header while the cache version
  // doesn't have one. Check that the new content is returned, and the cached
  // files is deleted.
  url_response = mojo::URLResponse::New();
  url_response->url = "http://www.example.com/3";
  DataPipe pipe5;
  new_content = base::RandBytesAsString(32);
  num_bytes = new_content.size();
  ASSERT_EQ(MOJO_RESULT_OK,
            WriteDataRaw(pipe5.producer_handle.get(), new_content.c_str(),
                         &num_bytes, MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));
  ASSERT_EQ(new_content.size(), num_bytes);
  pipe5.producer_handle.reset();
  url_response->body = pipe5.consumer_handle.Pass();
  url_response_disk_cache_->GetFile(
      url_response.Pass(),
      [&file, &cache_dir](Array<uint8_t> received_file_path,
                          Array<uint8_t> received_cache_dir_path) {
        file = toPath(received_file_path.Pass());
        cache_dir = toPath(received_cache_dir_path.Pass());
      });
  url_response_disk_cache_.WaitForIncomingResponse();
  ASSERT_FALSE(file.empty());
  ASSERT_TRUE(base::ReadFileToString(file, &received_content));
  EXPECT_EQ(new_content, received_content);
  saved_file = cache_dir.Append("file");
  EXPECT_FALSE(base::PathExists(saved_file));
}

}  // namespace mojo
