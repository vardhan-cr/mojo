// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.app.Activity;
import android.content.Intent;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;
import android.os.Bundle;
import android.os.Parcelable;

import org.chromium.mojo.application.ShellHelper;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.mojom.nfc.NfcMessageSink;

/**
 * Activity for receiving nfc messages.
 */
public class NfcReceiverActivity extends Activity {
    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        finish();
        // TODO(anwilson): Figure out how we can deliver nfc data when the shell is not active.
        if (!ShellMain.isInitialized()) {
            android.util.Log.w(
                    "Nfc", "MojoShell is not active; launching!  Try again in a moment.");
            startActivity(new Intent(this, MojoShellActivity.class));
            return;
        }
        sendNfcMessages(getIntent());
    }

    private static void sendNfcMessages(Intent intent) {
        NfcMessageSink nfcMessageSink = ShellHelper.connectToService(
                CoreImpl.getInstance(), ShellMain.getShell(), "mojo:nfc", NfcMessageSink.MANAGER);

        Parcelable[] rawMsgs = intent.getParcelableArrayExtra(NfcAdapter.EXTRA_NDEF_MESSAGES);

        // Only one message sent during the beam.
        NdefMessage msg = (NdefMessage) rawMsgs[0];

        // Records 0 to size-2 contains the MIME type, record size-1 is the AAR.
        NdefRecord[] records = msg.getRecords();
        for (int i = 0; i < (records.length - 1); i++) {
            byte[] payload = records[i].getPayload();
            if (payload != null) {
                nfcMessageSink.onNfcMessage(payload);
            }
        }
        nfcMessageSink.close();
    }
}
