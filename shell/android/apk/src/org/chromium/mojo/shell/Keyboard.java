// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.app.Activity;
import android.view.View;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

import org.chromium.mojo.keyboard.KeyboardServiceImpl;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.UntypedHandle;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.mojom.keyboard.KeyboardService;

/**
 * Interaction with the keyboard.
 */
@JNINamespace("shell")
public class Keyboard {
    @CalledByNative
    public static void createKeyboardImpl(Activity activity, int keyboardRequestHandleValue) {
        Core core = CoreImpl.getInstance();
        UntypedHandle keyboardRequestHandle = core.acquireNativeHandle(keyboardRequestHandleValue);
        MessagePipeHandle keyboardRequest = keyboardRequestHandle.toMessagePipeHandle();
        View v = activity.getCurrentFocus();
        KeyboardService.MANAGER.bind(
                new KeyboardServiceImpl(ApplicationStatus.getApplicationContext()),
                keyboardRequest);
        KeyboardServiceImpl.setActiveView(v);
    }
}
