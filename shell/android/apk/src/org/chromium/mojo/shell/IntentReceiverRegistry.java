// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.content.Intent;
import android.os.Parcel;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

/**
 * Java helper class for services/android/intent_receiver.mojom. This class creates intents for
 * privileged mojo applications and routes received intents back to the mojo application. The
 * implementation of the IntentReceiverManager service is in C++, but calls back to this class to
 * access the android framework.
 */
@JNINamespace("shell")
public class IntentReceiverRegistry {
    private static class LazyHolder {
        private static final IntentReceiverRegistry INSTANCE = new IntentReceiverRegistry();
    }

    /**
     * Returns the instance.
     */
    public static IntentReceiverRegistry getInstance() {
        return LazyHolder.INSTANCE;
    }

    private final Map<String, Long> mReceiversByUuid = new HashMap<>();
    private final Map<Long, String> mUuidsByReceiver = new HashMap<>();

    private IntentReceiverRegistry() {}

    public void onIntentReceived(Intent intent) {
        String uuid = intent.getAction();
        if (uuid == null) return;
        Long ptr = mReceiversByUuid.get(uuid);
        if (ptr == null) return;
        nativeOnIntentReceived(ptr, intentToBuffer(intent));
    }

    private static ByteBuffer intentToBuffer(Intent intent) {
        Parcel p = Parcel.obtain();
        intent.writeToParcel(p, 0);
        p.setDataPosition(0);
        byte[] b = p.marshall();
        ByteBuffer result = ByteBuffer.allocateDirect(b.length);
        result.put(b);
        result.flip();
        return result;
    }

    @CalledByNative
    private static ByteBuffer registerReceiver(long intentDispatcher) {
        IntentReceiverRegistry registry = getInstance();
        String uuid = UUID.randomUUID().toString();
        Intent intent = new Intent(
                uuid, null, ApplicationStatus.getApplicationContext(), IntentReceiverService.class);
        registry.mReceiversByUuid.put(uuid, intentDispatcher);
        registry.mUuidsByReceiver.put(intentDispatcher, uuid);
        return intentToBuffer(intent);
    }

    @CalledByNative
    private static void unregisterReceiver(long intentDispatcher) {
        IntentReceiverRegistry registry = getInstance();
        String uuid = registry.mUuidsByReceiver.get(intentDispatcher);
        if (uuid != null) {
            registry.mUuidsByReceiver.remove(intentDispatcher);
            registry.mReceiversByUuid.remove(uuid);
        }
    }

    private static native void nativeOnIntentReceived(long intentDispatcher, ByteBuffer intent);
}
