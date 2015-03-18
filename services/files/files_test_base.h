// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/services/files/public/interfaces/directory.mojom.h"
#include "mojo/services/files/public/interfaces/file.mojom.h"
#include "mojo/services/files/public/interfaces/files.mojom.h"
#include "mojo/services/files/public/interfaces/types.mojom.h"

namespace mojo {
namespace files {

// TODO(vtl): Stuff copied from mojo/public/cpp/bindings/lib/template_util.h.
template <class T, T v>
struct IntegralConstant {
  static const T value = v;
};

template <class T, T v>
const T IntegralConstant<T, v>::value;

typedef IntegralConstant<bool, true> TrueType;
typedef IntegralConstant<bool, false> FalseType;

template <class T>
struct IsConst : FalseType {};
template <class T>
struct IsConst<const T> : TrueType {};

template <bool B, typename T = void>
struct EnableIf {};

template <typename T>
struct EnableIf<true, T> {
  typedef T type;
};

typedef char YesType;

struct NoType {
  YesType dummy[2];
};

template <typename T>
struct IsMoveOnlyType {
  template <typename U>
  static YesType Test(const typename U::MoveOnlyTypeForCPP03*);

  template <typename U>
  static NoType Test(...);

  static const bool value =
      sizeof(Test<T>(0)) == sizeof(YesType) && !IsConst<T>::value;
};

template <typename T>
typename EnableIf<!IsMoveOnlyType<T>::value, T>::type& Forward(T& t) {
  return t;
}

template <typename T>
typename EnableIf<IsMoveOnlyType<T>::value, T>::type Forward(T& t) {
  return t.Pass();
}
// TODO(vtl): (End of stuff copied from template_util.h.)

template <typename T1>
Callback<void(T1)> Capture(T1* t1) {
  return [t1](T1 got_t1) { *t1 = Forward(got_t1); };
}

template <typename T1, typename T2>
Callback<void(T1, T2)> Capture(T1* t1, T2* t2) {
  return [t1, t2](T1 got_t1, T2 got_t2) {
    *t1 = Forward(got_t1);
    *t2 = Forward(got_t2);
  };
}

class FilesTestBase : public test::ApplicationTestBase {
 public:
  FilesTestBase() {}
  ~FilesTestBase() override {}

  void SetUp() override {
    test::ApplicationTestBase::SetUp();
    application_impl()->ConnectToService("mojo:files", &files_);
  }

 protected:
  // Note: This has an out parameter rather than returning the |DirectoryPtr|,
  // since |ASSERT_...()| doesn't work with return values.
  void GetTemporaryRoot(DirectoryPtr* directory) {
    Error error = ERROR_INTERNAL;
    files()->OpenFileSystem(FILE_SYSTEM_TEMPORARY, GetProxy(directory),
                            Capture(&error));
    ASSERT_TRUE(files().WaitForIncomingMethodCall());
    ASSERT_EQ(ERROR_OK, error);
  }

  FilesPtr& files() { return files_; }

 private:
  FilesPtr files_;

  DISALLOW_COPY_AND_ASSIGN(FilesTestBase);
};

}  // namespace files
}  // namespace mojo
