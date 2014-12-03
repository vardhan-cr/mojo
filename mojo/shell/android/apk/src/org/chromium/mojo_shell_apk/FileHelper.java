// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo_shell_apk;

import android.content.Context;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * Helper methods for file extraction from APK assets and zip archives.
 */
class FileHelper {
    // Size of the buffer used in streaming file operations.
    private static final int BUFFER_SIZE = 1024 * 1024;
    // Prefix used when naming temporary files.
    private static final String TEMP_FILE_PREFIX = "temp-";

    static File extractFromAssets(Context context, String assetName, File outputDirectory)
            throws IOException, FileNotFoundException {
        // Make the original filename part of the temp file name.
        // TODO(ppi): do we need to sanitize the suffix?
        String suffix = "-" + assetName;
        File outputFile = File.createTempFile(TEMP_FILE_PREFIX, suffix, outputDirectory);
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
    static File extractFromArchive(File archive, String suffixToMatch,
            File outputDirectory) throws IOException, FileNotFoundException {
        ZipInputStream zip = new ZipInputStream(new BufferedInputStream(new FileInputStream(
                archive)));
        ZipEntry entry;
        while ((entry = zip.getNextEntry()) != null) {
            if (entry.getName().endsWith(suffixToMatch)) {
                // Make the original filename part of the temp file name.
                // TODO(ppi): do we need to sanitize the suffix?
                String suffix = "-" + new File(entry.getName()).getName();
                File extractedFile = File.createTempFile(TEMP_FILE_PREFIX, suffix,
                        outputDirectory);
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
