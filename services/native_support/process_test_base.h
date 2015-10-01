// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_SUPPORT_PROCESS_TEST_BASE_H_
#define SERVICES_NATIVE_SUPPORT_PROCESS_TEST_BASE_H_

#include "base/macros.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/services/native_support/public/interfaces/process.mojom.h"

namespace native_support {

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
mojo::Callback<void(T1)> Capture(T1* t1) {
  return [t1](T1 got_t1) { *t1 = Forward(got_t1); };
}

template <typename T1, typename T2>
mojo::Callback<void(T1, T2)> Capture(T1* t1, T2* t2) {
  return [t1, t2](T1 got_t1, T2 got_t2) {
    *t1 = Forward(got_t1);
    *t2 = Forward(got_t2);
  };
}

class ProcessTestBase : public mojo::test::ApplicationTestBase {
 public:
  ProcessTestBase();
  ~ProcessTestBase() override;

  void SetUp() override;

 protected:
  ProcessPtr& process() { return process_; }

 private:
  ProcessPtr process_;

  DISALLOW_COPY_AND_ASSIGN(ProcessTestBase);
};

}  // namespace native_support

#endif  // SERVICES_NATIVE_SUPPORT_PROCESS_TEST_BASE_H_
