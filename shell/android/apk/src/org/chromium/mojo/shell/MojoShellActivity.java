// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.Bundle;

import java.util.ArrayDeque;

/**
 * Entry point for the Mojo Shell application.
 */
public class MojoShellActivity extends Activity implements ShellService.IShellBindingActivity {
    private ArrayDeque<Intent> mPendingIntents = new ArrayDeque<Intent>();
    private ShellService mShellService;
    private ServiceConnection mShellServiceConnection;

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent serviceIntent = new Intent(this, ShellService.class);
        // Copy potential startup arguments.
        serviceIntent.putExtras(getIntent());
        startService(serviceIntent);

        mShellServiceConnection = new ShellService.ShellServiceConnection(this);
        bindService(serviceIntent, mShellServiceConnection, Context.BIND_AUTO_CREATE);

        onNewIntent(getIntent());
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        mPendingIntents.add(intent);
    }

    @Override
    public void onShellBound(ShellService shellService) {
        mShellService = shellService;
        finish();
        communicateWithShell();
        unbindService(mShellServiceConnection);
    }

    @Override
    public void onShellUnbound() {
        mShellService = null;
        mShellServiceConnection = null;
    }

    /**
     * Communicate with the shell to start new apps, based on pending intents.
     */
    private void communicateWithShell() {
        while (!mPendingIntents.isEmpty()) {
            Intent intent = mPendingIntents.remove();
            Uri data = intent.getData();
            if (data != null) {
                String url = data.buildUpon().scheme("https").build().toString();
                mShellService.startApplicationURL(url);
            }
        }
    }
}
