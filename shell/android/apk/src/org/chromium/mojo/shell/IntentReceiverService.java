// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.app.IntentService;
import android.content.Intent;

import org.chromium.base.ApplicationStatus;

/**
 * IntentService that will forward received intents to the {@link IntentReceiverRegistry}.
 */
public class IntentReceiverService extends IntentService {
    public static final String CATEGORY_START_ACTIVITY_FOR_RESULT = "C_startActivityForResult";
    public static final String EXTRA_START_ACTIVITY_FOR_RESULT_INTENT = "intent";

    public IntentReceiverService() {
        super("IntentReceiverService");
    }

    /**
     * @see IntentService#onHandleIntent(Intent)
     */
    @Override
    protected void onHandleIntent(Intent intent) {
        if (intent.getCategories() != null
                && intent.getCategories().contains(CATEGORY_START_ACTIVITY_FOR_RESULT)) {
            if (!intent.hasExtra(EXTRA_START_ACTIVITY_FOR_RESULT_INTENT)) {
                return;
            }
            ApplicationStatus.getLastTrackedFocusedActivity().startActivityForResult(
                    intent.<Intent>getParcelableExtra(EXTRA_START_ACTIVITY_FOR_RESULT_INTENT),
                    Integer.parseInt(intent.getAction()));
            return;
        }
        IntentReceiverRegistry.getInstance().onIntentReceived(intent);
    }
}
