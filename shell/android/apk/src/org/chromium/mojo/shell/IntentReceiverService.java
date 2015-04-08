// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.app.IntentService;
import android.content.Intent;

/**
 * IntentService that will forward received intents to the {@link IntentReceiverRegistry}.
 */
public class IntentReceiverService extends IntentService {
    public IntentReceiverService() {
        super("IntentReceiverService");
    }

    /**
     * @see IntentService#onHandleIntent(Intent)
     */
    @Override
    protected void onHandleIntent(Intent intent) {
        IntentReceiverRegistry.getInstance().onIntentReceived(intent);
    }
}
