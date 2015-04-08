// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/intent_receiver_manager_impl.h"

#include "base/android/jni_android.h"
#include "jni/IntentReceiverRegistry_jni.h"
#include "mojo/public/cpp/bindings/error_handler.h"

namespace mojo {
namespace shell {

namespace {

mojo::Array<uint8> BufferToArray(JNIEnv* env, jobject buffer) {
  const size_t size = env->GetDirectBufferCapacity(buffer);
  Array<uint8> result(size);
  memcpy(&result.front(), env->GetDirectBufferAddress(buffer), size);
  return result.Pass();
}

class IntentDispatcher : public ErrorHandler {
 public:
  IntentDispatcher(intent_receiver::IntentReceiverPtr intent_receiver)
      : intent_receiver_(intent_receiver.Pass()) {
    intent_receiver_.set_error_handler(this);
  }

  ~IntentDispatcher() {
    Java_IntentReceiverRegistry_unregisterReceiver(
        base::android::AttachCurrentThread(),
        reinterpret_cast<uintptr_t>(this));
  }

  void OnIntentReceived(JNIEnv* env, jobject intent) {
    intent_receiver_->OnIntent(BufferToArray(env, intent));
  }

 private:
  // Overriden from ErrorHandler
  void OnConnectionError() {
    intent_receiver_.set_error_handler(nullptr);
    delete this;
  }

  intent_receiver::IntentReceiverPtr intent_receiver_;
};

}  // namespace

void IntentReceiverManagerImpl::Bind(
    InterfaceRequest<intent_receiver::IntentReceiverManager> request) {
  bindings_.AddBinding(this, request.Pass());
}

void IntentReceiverManagerImpl::RegisterReceiver(
    intent_receiver::IntentReceiverPtr receiver,
    const RegisterReceiverCallback& callback) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> buffer =
      Java_IntentReceiverRegistry_registerReceiver(
          env,
          reinterpret_cast<uintptr_t>(new IntentDispatcher(receiver.Pass())));
  callback.Run(BufferToArray(env, buffer.obj()));
}

bool RegisterIntentReceiverRegistry(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

void OnIntentReceived(JNIEnv* env,
                      jclass jcaller,
                      jlong intent_dispatcher_ptr,
                      jobject intent) {
  IntentDispatcher* intent_dispatcher =
      reinterpret_cast<IntentDispatcher*>(intent_dispatcher_ptr);
  intent_dispatcher->OnIntentReceived(env, intent);
}

}  // namespace shell
}  // namespace mojo
