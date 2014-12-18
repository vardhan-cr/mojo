// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo_shell_apk;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.widget.EditText;

/**
 * Activity for managing the Mojo Shell.
 */
public class MojoShellActivity extends Activity {
    private static final String TAG = "MojoShellActivity";

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // TODO(ppi): Gotcha - the call below will work only once per process lifetime, but the OS
        // has no obligation to kill the application process between destroying and restarting the
        // activity. If the application process is kept alive, initialization parameters sent with
        // the intent will be stale.
        // TODO(qsr): We should be passing application context here as required by
        // InitApplicationContext on the native side. Currently we can't, as PlatformViewportAndroid
        // relies on this being the activity context.
        MojoMain.ensureInitialized(this, getParametersFromIntent(getIntent()));

        if (MojoMain.start()) {
            Log.i(TAG, "Mojo started");
        } else {
            Log.i(TAG, "No URL provided via intent, prompting user...");
            AlertDialog.Builder alert = new AlertDialog.Builder(this);
            alert.setTitle("Enter a URL");
            alert.setMessage("Enter a URL");
            final EditText input = new EditText(this);
            alert.setView(input);
            alert.setPositiveButton("Load", new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int button) {
                    String url = input.getText().toString();
                    MojoMain.addApplicationURL(url);
                    MojoMain.start();
                    Log.i(TAG, "Mojo started");
                }
            });
            alert.show();
        }
    }

    private static String[] getParametersFromIntent(Intent intent) {
        return intent != null ? intent.getStringArrayExtra("parameters") : null;
    }

    private void startWithURL(String url) {
    }
}
