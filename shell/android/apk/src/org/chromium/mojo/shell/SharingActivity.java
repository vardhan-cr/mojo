// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;

import org.chromium.mojo.application.ShellHelper;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.mojom.sharing.SharingSink;

/**
 * Activity for sharing data with shell apps.
 */
public final class SharingActivity extends Activity {
    private static final String TAG = "SharingActivity";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Not showing any UI, so just finish this activity before doing anything else.
        finish();

        // TODO(pylaligand): Figure out how we can deliver share data when the shell is not active.
        if (!ShellMain.isInitialized()) {
            Log.w(TAG, "MojoShell is not active; launching! Try sharing again in a moment.");
            startActivity(new Intent(this, MojoShellActivity.class));
            return;
        }

        String action = getIntent().getAction();
        if (!Intent.ACTION_SEND.equals(action)) {
            Log.w(TAG, "Wrong action sent to this activity: " + action + ".");
            return;
        }

        String type = getIntent().getType();
        if (!"text/plain".equals(type)) {
            Log.w(TAG, "Wrong data type sent to this activity: " + type + ".");
            return;
        }

        String text = getIntent().getStringExtra(Intent.EXTRA_TEXT);
        if (TextUtils.isEmpty(text)) {
            Log.w(TAG, "No text available to share.");
            return;
        }

        SharingSink sink = ShellHelper.connectToService(
                CoreImpl.getInstance(), ShellMain.getShell(), "mojo:sharing", SharingSink.MANAGER);
        sink.onTextShared(text);
    }
}
