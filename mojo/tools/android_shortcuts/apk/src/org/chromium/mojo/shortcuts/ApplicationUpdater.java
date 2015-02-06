// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shortcuts;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Environment;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

/**
 * Helper class to update apks.
 */
public class ApplicationUpdater {
    private static final String TAG = "ApplicationUpdater";

    private static class ApplicationLocation {
        private final String mPackageName;
        private final String mURL;

        public ApplicationLocation(String packageName, String url) {
            mPackageName = packageName;
            mURL = url;
        }

        /**
         * Returns the application package.
         */
        public String getPackageName() {
            return mPackageName;
        }

        /**
         * Returns the application update URL.
         */
        public URL getURL() throws MalformedURLException {
            return new URL(mURL);
        }
    }

    private static final ApplicationLocation[] APPLICATIONS = {
            new ApplicationLocation(
                    "org.chromium.mojo.shortcuts", "http://domokit.github.io/MojoShortcuts.apk"),
            new ApplicationLocation(
                    "org.chromium.mojo.shell", "http://domokit.github.io/MojoShell.apk")};

    private static boolean applicationNeedsUpdate(
            Context context, ApplicationLocation application) {
        PackageManager pm = context.getPackageManager();
        try {
            PackageInfo pi = pm.getPackageInfo(application.getPackageName(), 0);
            HttpURLConnection urlConnection =
                    (HttpURLConnection) application.getURL().openConnection();
            urlConnection.setRequestMethod("HEAD");
            urlConnection.connect();
            long application_timestamp = urlConnection.getHeaderFieldDate("Last-Modified", 0);
            return application_timestamp > pi.lastUpdateTime;
        } catch (PackageManager.NameNotFoundException e) {
            return true;
        } catch (IOException e) {
            Log.e(TAG,
                    "Unable to retrieve information for package: " + application.getPackageName(),
                    e);
        }
        return false;
    }

    private static void updateApplication(Context context, ApplicationLocation application) {
        try {
            URL url = application.getURL();
            HttpURLConnection c = (HttpURLConnection) url.openConnection();
            c.setRequestMethod("GET");
            c.connect();

            File file = new File(Environment.getExternalStorageDirectory(), "download");
            file.mkdirs();
            File outputFile = new File(file, "update.apk");

            FileOutputStream fos = new FileOutputStream(outputFile);
            try {
                InputStream is = c.getInputStream();
                try {
                    byte[] buffer = new byte[4096];
                    int len1 = 0;
                    while ((len1 = is.read(buffer)) != -1) {
                        fos.write(buffer, 0, len1);
                    }
                } finally {
                    is.close();
                }
            } finally {
                fos.close();
            }

            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setDataAndType(
                    Uri.fromFile(outputFile), "application/vnd.android.package-archive");
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
        } catch (IOException e) {
            Log.e(TAG, "Unable to update package: " + application.getPackageName(), e);
        }
    }

    public static void checkAndUpdateApplications(Context context) {
        for (ApplicationLocation application : APPLICATIONS) {
            if (applicationNeedsUpdate(context, application)) {
                updateApplication(context, application);
            }
        }
    }
}
