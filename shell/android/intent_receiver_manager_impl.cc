// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/intent_receiver_manager_impl.h"

#include "base/android/jni_android.h"
#include "jni/IntentReceiverRegistry_jni.h"
#include "mojo/public/cpp/bindings/error_handler.h"

namespace shell {

namespace {

mojo::Array<uint8_t> BufferToArray(JNIEnv* env, jobject buffer) {
  const size_t size = env->GetDirectBufferCapacity(buffer);
  mojo::Array<uint8_t> result(size);
  memcpy(&result.front(), env->GetDirectBufferAddress(buffer), size);
  return result.Pass();
}

class IntentDispatcher : public mojo::ErrorHandler {
 public:
  enum LifeCycle { Persistent, OneShot };
  IntentDispatcher(intent_receiver::IntentReceiverPtr intent_receiver,
                   LifeCycle life_cycle)
      : intent_receiver_(intent_receiver.Pass()), life_cycle_(life_cycle) {
    intent_receiver_.set_error_handler(this);
  }

  ~IntentDispatcher() {
    Java_IntentReceiverRegistry_unregisterReceiver(
        base::android::AttachCurrentThread(),
        reinterpret_cast<uintptr_t>(this));
  }

  void OnIntentReceived(JNIEnv* env, jobject intent) {
    intent_receiver_->OnIntent(BufferToArray(env, intent));
    if (life_cycle_ == OneShot)
      delete this;
  }

 private:
  // Overriden from mojo::ErrorHandler
  void OnConnectionError() {
    intent_receiver_.set_error_handler(nullptr);
    delete this;
  }

  intent_receiver::IntentReceiverPtr intent_receiver_;
  LifeCycle life_cycle_;
};

}  // namespace

void IntentReceiverManagerImpl::Bind(
    mojo::InterfaceRequest<intent_receiver::IntentReceiverManager> request) {
  bindings_.AddBinding(this, request.Pass());
}

void IntentReceiverManagerImpl::RegisterIntentReceiver(
    intent_receiver::IntentReceiverPtr receiver,
    const RegisterIntentReceiverCallback& callback) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> buffer =
      Java_IntentReceiverRegistry_registerIntentReceiver(
          env, reinterpret_cast<uintptr_t>(new IntentDispatcher(
                   receiver.Pass(), IntentDispatcher::Persistent)));
  callback.Run(BufferToArray(env, buffer.obj()));
}

void IntentReceiverManagerImpl::RegisterActivityResultReceiver(
    intent_receiver::IntentReceiverPtr receiver,
    const RegisterActivityResultReceiverCallback& callback) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> buffer =
      Java_IntentReceiverRegistry_registerActivityResultReceiver(
          env, reinterpret_cast<uintptr_t>(new IntentDispatcher(
                   receiver.Pass(), IntentDispatcher::Persistent)));
  callback.Run(BufferToArray(env, buffer.obj()));
}

bool RegisterIntentReceiverRegistry(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

void OnIntentReceived(JNIEnv* env,
                      jclass jcaller,
                      jlong intent_dispatcher_ptr,
                      jboolean success,
                      jobject intent) {
  IntentDispatcher* intent_dispatcher =
      reinterpret_cast<IntentDispatcher*>(intent_dispatcher_ptr);
  if (success) {
  intent_dispatcher->OnIntentReceived(env, intent);
  } else {
    delete intent_dispatcher;
  }
}

}  // namespace shell
