// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo_shell_apk;

import org.chromium.base.JNINamespace;

import java.io.File;

/**
 * Runnable used to bootstrap execution of Android Mojo application. For the JNI to work, we need a
 * Java class with the application classloader in the call stack. We load this class in the
 * application classloader and call into native from it to achieve that.
 */
@JNINamespace("mojo")
public class Bootstrap implements Runnable {

    private final File mBootstrapNativeLibrary;
    private final File mApplicationNativeLibrary;
    private final int mHandle;

    public Bootstrap(File bootstrapNativeLibrary, File applicationNativeLibrary, Integer handle) {
        mBootstrapNativeLibrary = bootstrapNativeLibrary;
        mApplicationNativeLibrary = applicationNativeLibrary;
        mHandle = handle;
    }

    @Override
    public void run() {
        System.load(mBootstrapNativeLibrary.getAbsolutePath());
        System.load(mApplicationNativeLibrary.getAbsolutePath());
        nativeBootstrap(mApplicationNativeLibrary.getAbsolutePath(), mHandle);
    }

    native void nativeBootstrap(String libraryPath, int handle);
}
