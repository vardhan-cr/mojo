// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.app.Activity;
import android.app.UiModeManager;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.view.WindowManager;

import java.util.ArrayDeque;

/**
 * Activity for managing the Mojo Shell.
 */
public class MojoShellActivity extends Activity implements ShellService.IShellBindingActivity {
    private static final String TAG = "MojoShellActivity";
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
        bindShellService(serviceIntent);

        onNewIntent(getIntent());

        // TODO(tonyg): Watch activities go back to the home screen within a
        // couple of seconds of detaching from adb. So for demonstration purposes,
        // we just keep the screen on. Eventually we'll want a solution for
        // allowing the screen to sleep without quitting the shell.
        // TODO(etiennej): Verify the above is still true after the switch to a Service model.
        UiModeManager uiModeManager = (UiModeManager) getSystemService(UI_MODE_SERVICE);
        if (uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_WATCH) {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }
    }

    private void bindShellService(Intent serviceIntent) {
        if (mShellServiceConnection == null) {
            mShellServiceConnection = new ShellService.ShellServiceConnection(this);
            bindService(serviceIntent, mShellServiceConnection, Context.BIND_AUTO_CREATE);
        }
    }

    private void unbindShellService() {
        if (mShellServiceConnection != null) {
            unbindService(mShellServiceConnection);
            mShellServiceConnection = null;
        }
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        bindShellService(new Intent(this, ShellService.class));
    }

    @Override
    protected void onStop() {
        super.onStop();
        unbindShellService();
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        mPendingIntents.add(intent);

        if (mShellService != null) {
            communicateWithShell();
        }
    }

    @Override
    public void onShellBound(ShellService shellService) {
        mShellService = shellService;
        communicateWithShell();
    }

    @Override
    public void onShellUnbound() {
        mShellService = null;
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

    /**
     * @see Activity#onActivityResult(int, int, Intent)
     */
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        IntentReceiverRegistry.getInstance().onActivityResult(requestCode, resultCode, data);
    }
}
