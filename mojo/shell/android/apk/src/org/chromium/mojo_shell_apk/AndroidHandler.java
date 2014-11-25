// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo_shell_apk;

import android.content.Context;
import android.util.Log;

import dalvik.system.DexClassLoader;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Constructor;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * Content handler for archives containing native libraries bundled with Java code.
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
    private static final String APP_LIBRARY_PREFIX = "app-library";
    private static final String ARCHIVE_SUFFIX = ".zip";

    // Directories used to hold temporary files. These are cleared when clearTemporaryFiles() is
    // called.
    private static final String DEX_OUTPUT_DIRECTORY = "dex_output";
    private static final String APP_DIRECTORY = "applications";
    private static final String ASSET_DIRECTORY = "assets";

    // Size of the buffer used in streaming file operations.
    private static final int BUFFER_SIZE = 1024 * 1024;

    /**
     * Deletes directories holding the temporary files. This should be called early on shell
     * startup to clean up after the previous run.
     */
    static void clearTemporaryFiles(Context context) {
        context.getDir(DEX_OUTPUT_DIRECTORY, Context.MODE_PRIVATE).delete();
        context.getDir(APP_DIRECTORY, Context.MODE_PRIVATE).delete();
        context.getDir(ASSET_DIRECTORY, Context.MODE_PRIVATE).delete();
    }

    /**
     * Returns the path at which the native part should save the application archive.
     */
    @CalledByNative
    private static String getNewTempArchivePath(Context context) throws IOException {
        return File.createTempFile(ARCHIVE_PREFIX, ARCHIVE_SUFFIX,
                context.getDir(APP_DIRECTORY, Context.MODE_PRIVATE)).getAbsolutePath();
    }

    /**
     * Extracts and runs the application libraries contained by the indicated archive.
     */
    @CalledByNative
    private static boolean bootstrap(Context context, String archivePath, int handle) {
        File bootstrap_java_library;
        File bootstrap_native_library;
        try {
            bootstrap_java_library = extractFromAssets(context, BOOTSTRAP_JAVA_LIBRARY);
            bootstrap_native_library = extractFromAssets(context, BOOTSTRAP_NATIVE_LIBRARY);
        } catch (Exception e) {
            Log.e(TAG, "Extraction of bootstrap files from assets failed: " + e);
            return false;
        }

        File application_java_library;
        File application_native_library;
        try {
            File archive = new File(archivePath);
            application_java_library = extractFromArchive(context, archive, JAVA_LIBRARY_SUFFIX);
            application_native_library = extractFromArchive(context, archive,
                    NATIVE_LIBRARY_SUFFIX);
        } catch (Exception e) {
            Log.e(TAG, "Extraction of application files from the archive failed: " + e);
            return false;
        }

        String dexPath = bootstrap_java_library.getAbsolutePath() + File.pathSeparator
                + application_java_library.getAbsolutePath();
        File dex_output_dir = context.getDir(DEX_OUTPUT_DIRECTORY, Context.MODE_PRIVATE);
        DexClassLoader bootstrapLoader = new DexClassLoader(dexPath,
                dex_output_dir.getAbsolutePath(), null, ClassLoader.getSystemClassLoader());

        try {
            Class<?> loadedClass = bootstrapLoader.loadClass(BOOTSTRAP_CLASS);
            Class<? extends Runnable> bootstrapClass = loadedClass.asSubclass(Runnable.class);
            Constructor<? extends Runnable> constructor = bootstrapClass.getConstructor(
                    File.class, File.class, Integer.class);
            Runnable bootstrapRunnable = constructor.newInstance(bootstrap_native_library,
                    application_native_library, Integer.valueOf(handle));
            bootstrapRunnable.run();
        } catch (Throwable t) {
            Log.e(TAG, "Running Bootstrap failed: " + t);
            return false;
        }
        return true;
    }

    private static File extractFromAssets(Context context, String assetName)
            throws IOException, FileNotFoundException {
        File outputFile = new File(
                context.getDir(ASSET_DIRECTORY, Context.MODE_PRIVATE), assetName);

        BufferedInputStream inputStream = new BufferedInputStream(
                context.getAssets().open(assetName));
        writeStreamToFile(inputStream, outputFile);
        inputStream.close();
        return outputFile;
    }

    /**
     * Extracts the file of the given extension from the archive. Throws FileNotFoundException if
     * no matching file is found.
     */
    private static File extractFromArchive(Context context, File archive, String suffix)
            throws IOException, FileNotFoundException {
        ZipInputStream zip = new ZipInputStream(new BufferedInputStream(new FileInputStream(
                archive)));
        ZipEntry entry;
        while ((entry = zip.getNextEntry()) != null) {
            if (entry.getName().endsWith(suffix)) {
                String fileName = new File(entry.getName()).getName();
                File extractedFile = File.createTempFile(APP_LIBRARY_PREFIX, suffix,
                        context.getDir(APP_DIRECTORY, Context.MODE_PRIVATE));
                writeStreamToFile(zip, extractedFile);
                zip.close();
                return extractedFile;
            }
        }
        zip.close();
        throw new FileNotFoundException();
    }

    private static void writeStreamToFile(InputStream inputStream, File outputFile)
            throws IOException {
        byte[] buffer = new byte[BUFFER_SIZE];
        OutputStream outputStream = new BufferedOutputStream(new FileOutputStream(outputFile));
        int read;
        while ((read = inputStream.read(buffer, 0, BUFFER_SIZE)) > 0) {
            outputStream.write(buffer, 0, read);
        }
        outputStream.close();
    }
}
