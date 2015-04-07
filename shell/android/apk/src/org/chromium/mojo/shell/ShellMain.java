// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.app.Activity;
import android.content.Context;
import android.util.Log;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * A placeholder class to call native functions.
 **/
@JNINamespace("mojo::shell")
public class ShellMain {
    private static final String TAG = "ShellMain";

    // Directory where applications bundled with the shell will be extracted.
    private static final String LOCAL_APP_DIRECTORY = "local_apps";
    // Individual applications bundled with the shell as assets.
    private static final String NETWORK_LIBRARY_APP = "network_service.mojo";
    // Not really an executable, but what we'll use for "argv[0]" (along with the path).
    private static final String MOJO_SHELL_EXECUTABLE = "libmojo_shell.so";
    // Directory where the child executable will be extracted.
    private static final String CHILD_DIRECTORY = "child";
    // Name of the child executable.
    private static final String MOJO_SHELL_CHILD_EXECUTABLE = "mojo_shell_child";

    /**
     * A guard flag for calling nativeInit() only once.
     **/
    private static boolean sInitialized = false;

    /**
     * Initializes the native system.
     **/
    static void ensureInitialized(Context applicationContext, String[] parameters) {
        if (sInitialized) return;
        try {
            FileHelper.extractFromAssets(applicationContext, NETWORK_LIBRARY_APP,
                    getLocalAppsDir(applicationContext), false);
            File mojoShell = new File(applicationContext.getApplicationInfo().nativeLibraryDir,
                    MOJO_SHELL_EXECUTABLE);
            FileHelper.extractFromAssets(applicationContext, MOJO_SHELL_CHILD_EXECUTABLE,
                    getChildDir(applicationContext), false);
            File mojoShellChild =
                    new File(getChildDir(applicationContext), MOJO_SHELL_CHILD_EXECUTABLE);
            // The shell child executable needs to be ... executable.
            mojoShellChild.setExecutable(true, true);

            List<String> parametersList = new ArrayList<String>();
            // Program name.
            if (parameters != null) {
                parametersList.addAll(Arrays.asList(parameters));
            }

            nativeInit(applicationContext, mojoShell.getAbsolutePath(),
                    mojoShellChild.getAbsolutePath(),
                    parametersList.toArray(new String[parametersList.size()]),
                    getLocalAppsDir(applicationContext).getAbsolutePath(),
                    getTmpDir(applicationContext).getAbsolutePath());
            sInitialized = true;
        } catch (Exception e) {
            Log.e(TAG, "ShellMain initialization failed.", e);
            throw new RuntimeException(e);
        }
    }

    /**
     * Starts the specified application in the specified context.
     *
     * @return <code>true</code> if an application has been launched.
     **/
    static boolean start() {
        return nativeStart();
    }

    /**
     * Adds the given URL to the set of mojo applications to run on start.
     */
    static void addApplicationURL(String url) {
        nativeAddApplicationURL(url);
    }

    private static File getLocalAppsDir(Context context) {
        return context.getDir(LOCAL_APP_DIRECTORY, Context.MODE_PRIVATE);
    }

    private static File getChildDir(Context context) {
        return context.getDir(CHILD_DIRECTORY, Context.MODE_PRIVATE);
    }

    private static File getTmpDir(Context context) {
        return new File(context.getCacheDir(), "tmp");
    }

    @CalledByNative
    private static void finishActivity(Activity activity) {
        activity.finish();
    }

    /**
     * Initializes the native system. This API should be called only once per process.
     **/
    private static native void nativeInit(Context context, String mojoShellPath,
            String mojoShellChildPath, String[] parameters, String bundledAppsDirectory,
            String tmpDir);

    private static native boolean nativeStart();

    private static native void nativeAddApplicationURL(String url);
}
