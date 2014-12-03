// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo_shell_apk;

import android.content.Context;
import android.util.Log;

import dalvik.system.DexClassLoader;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Constructor;

/**
 * Content handler for archives containing native libraries bundled with Java code.
 * TODO(ppi): create a seperate instance for each application being bootstrapped to keep track of
 * the temporary files and clean them up once the execution finishes.
 */
@JNINamespace("mojo")
public class AndroidHandler {
    private static final String TAG = "AndroidHandler";

    // Bootstrap native and java libraries are packaged with the MojoShell APK as assets.
    private static final String BOOTSTRAP_JAVA_LIBRARY = "bootstrap_java.dex.jar";
    private static final String BOOTSTRAP_NATIVE_LIBRARY = "libbootstrap.so";
    // Name of the bootstrapping runnable shipped in the packaged Java library.
    private static final String BOOTSTRAP_CLASS = "org.chromium.mojo_shell_apk.Bootstrap";

    // File extensions used to identify application libraries in the provided archive.
    private static final String JAVA_LIBRARY_SUFFIX = ".dex.jar";
    private static final String NATIVE_LIBRARY_SUFFIX = ".so";
    // Filename sections used for naming temporary files holding application files.
    private static final String ARCHIVE_PREFIX = "archive";
    private static final String ARCHIVE_SUFFIX = ".zip";

    // Directories used to hold temporary files. These are cleared when clearTemporaryFiles() is
    // called.
    private static final String DEX_OUTPUT_DIRECTORY = "dex_output";
    private static final String APP_DIRECTORY = "applications";
    private static final String ASSET_DIRECTORY = "assets";

    /**
     * Deletes directories holding the temporary files. This should be called early on shell
     * startup to clean up after the previous run.
     */
    static void clearTemporaryFiles(Context context) {
        getDexOutputDir(context).delete();
        getAppDir(context).delete();
        getAssetDir(context).delete();
    }

    /**
     * Returns the path at which the native part should save the application archive.
     */
    @CalledByNative
    private static String getNewTempArchivePath(Context context) throws IOException {
        return File.createTempFile(ARCHIVE_PREFIX, ARCHIVE_SUFFIX,
                getAppDir(context)).getAbsolutePath();
    }

    /**
     * Extracts and runs the application libraries contained by the indicated archive.
     */
    @CalledByNative
    private static boolean bootstrap(Context context, String archivePath, int handle) {
        File bootstrap_java_library;
        File bootstrap_native_library;
        try {
            bootstrap_java_library = FileHelper.extractFromAssets(context, BOOTSTRAP_JAVA_LIBRARY,
                    getAssetDir(context));
            bootstrap_native_library = FileHelper.extractFromAssets(context,
                    BOOTSTRAP_NATIVE_LIBRARY, getAssetDir(context));
        } catch (Exception e) {
            Log.e(TAG, "Extraction of bootstrap files from assets failed: " + e);
            return false;
        }

        File application_java_library;
        File application_native_library;
        try {
            File archive = new File(archivePath);
            application_java_library = FileHelper.extractFromArchive(archive, JAVA_LIBRARY_SUFFIX,
                    getAppDir(context));
            application_native_library = FileHelper.extractFromArchive(archive,
                    NATIVE_LIBRARY_SUFFIX, getAppDir(context));
        } catch (Exception e) {
            Log.e(TAG, "Extraction of application files from the archive failed: " + e);
            return false;
        }

        String dexPath = bootstrap_java_library.getAbsolutePath() + File.pathSeparator
                + application_java_library.getAbsolutePath();
        DexClassLoader bootstrapLoader = new DexClassLoader(dexPath,
                getDexOutputDir(context).getAbsolutePath(), null,
                ClassLoader.getSystemClassLoader());

        try {
            Class<?> loadedClass = bootstrapLoader.loadClass(BOOTSTRAP_CLASS);
            Class<? extends Runnable> bootstrapClass = loadedClass.asSubclass(Runnable.class);
            Constructor<? extends Runnable> constructor = bootstrapClass.getConstructor(
                    Context.class, File.class, File.class, Integer.class);
            Runnable bootstrapRunnable = constructor.newInstance(context, bootstrap_native_library,
                    application_native_library, Integer.valueOf(handle));
            bootstrapRunnable.run();
        } catch (Throwable t) {
            Log.e(TAG, "Running Bootstrap failed: " + t);
            return false;
        }
        return true;
    }

    private static File getDexOutputDir(Context context) {
        return context.getDir(DEX_OUTPUT_DIRECTORY, Context.MODE_PRIVATE);
    }

    private static File getAppDir(Context context) {
        return context.getDir(APP_DIRECTORY, Context.MODE_PRIVATE);
    }

    private static File getAssetDir(Context context) {
        return context.getDir(ASSET_DIRECTORY, Context.MODE_PRIVATE);
    }
}
