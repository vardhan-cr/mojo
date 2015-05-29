// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.authentication;

import android.accounts.AccountManager;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Parcel;

import com.google.android.gms.auth.GoogleAuthException;
import com.google.android.gms.auth.GoogleAuthUtil;
import com.google.android.gms.auth.UserRecoverableAuthException;
import com.google.android.gms.common.AccountPicker;

import org.chromium.mojo.application.ShellHelper;
import org.chromium.mojo.bindings.SideEffectFreeCloseable;
import org.chromium.mojo.intent.IntentReceiver;
import org.chromium.mojo.intent.IntentReceiverManager;
import org.chromium.mojo.intent.IntentReceiverManager.RegisterActivityResultReceiverResponse;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojom.mojo.Shell;

import java.io.IOException;

/**
 * Implementation of AuthenticationService from services/authentication/authentication.mojom
 */
public class AuthenticationServiceImpl
        extends SideEffectFreeCloseable implements AuthenticationService {
    /**
     * An callback that takes a serialized intent, add the intent the shell needs to send and start
     * the container intent.
     */
    private final class RegisterActivityResultReceiverCallback
            implements RegisterActivityResultReceiverResponse {
        /**
         * The intent that the requesting application needs to be run by shell on its
behalf.
         */
        private final Intent mIntent;

        private RegisterActivityResultReceiverCallback(Intent intent) {
            mIntent = intent;
        }

        /**
         * @see RegisterActivityResultReceiverResponse#call(byte[])
         */
        @Override
        public void call(byte[] serializedIntent) {
            Intent trampolineIntent = bytesToIntent(serializedIntent);
            trampolineIntent.putExtra("intent", mIntent);
            mContext.startService(trampolineIntent);
        }
    }

    private final Activity mContext;
    private final String mConsumerURL;
    private final IntentReceiverManager mIntentReceiverManager;

    public AuthenticationServiceImpl(Context context, Core core, String consumerURL, Shell shell) {
        mContext = (Activity) context;
        mConsumerURL = consumerURL;
        mIntentReceiverManager = ShellHelper.connectToService(
                core, shell, "mojo:android_handler", IntentReceiverManager.MANAGER);
    }

    /**
     * @see AuthenticationService#onConnectionError(MojoException)
     */
    @Override
    public void onConnectionError(MojoException e) {}

    /**
     * @see AuthenticationService#getOAuth2Token(String, String[],
     *      AuthenticationService.GetOAuth2TokenResponse)
     */
    @Override
    public void getOAuth2Token(
            final String username, final String[] scopes, final GetOAuth2TokenResponse callback) {
        if (scopes.length == 0) {
            callback.call(null, "scopes cannot be empty");
            return;
        }
        StringBuilder scope = new StringBuilder("oauth2:");
        for (int i = 0; i < scopes.length; ++i) {
            if (i > 0) {
                scope.append(" ");
            }
            scope.append(scopes[i]);
        }
        try {
            callback.call(GoogleAuthUtil.getToken(mContext, username, scope.toString()), null);
        } catch (final UserRecoverableAuthException e) {
            // If an error occurs that the user can recover from, this exception will contain the
            // intent to run to allow the user to act. This will use the intent manager to start it.
            mIntentReceiverManager.registerActivityResultReceiver(new IntentReceiver() {
                GetOAuth2TokenResponse mPendingCallback = callback;

                @Override
                public void close() {
                    call(null, "User denied the request.");
                }

                @Override
                public void onConnectionError(MojoException e) {
                    call(null, e.getMessage());
                }

                @Override
                public void onIntent(byte[] bytes) {
                    if (mPendingCallback == null) {
                        return;
                    }
                    getOAuth2Token(username, scopes, mPendingCallback);
                    mPendingCallback = null;
                }

                private void call(String token, String error) {
                    if (mPendingCallback == null) {
                        return;
                    }
                    mPendingCallback.call(token, error);
                    mPendingCallback = null;
                }
            }, new RegisterActivityResultReceiverCallback(e.getIntent()));
            return;
        } catch (IOException | GoogleAuthException e) {
            // Unrecoverable error.
            callback.call(null, e.getMessage());
        }
    }

    /**
     * @see AuthenticationService#selectAccount(AuthenticationService.SelectAccountResponse)
     */
    @Override
    public void selectAccount(final SelectAccountResponse callback) {
        String[] accountTypes = new String[] {"com.google"};
        String message = null;
        if (mConsumerURL.equals("")) {
            message = "Select an account to use with mojo shell";
        } else {
            message = "Select an account to use with application: " + mConsumerURL;
        }
        Intent accountPickerIntent = AccountPicker.newChooseAccountIntent(
                null, null, accountTypes, false, message, null, null, null);

        mIntentReceiverManager.registerActivityResultReceiver(new IntentReceiver() {
            SelectAccountResponse mPendingCallback = callback;

            @Override
            public void close() {
                call(null, "User denied the request.");
            }

            @Override
            public void onConnectionError(MojoException e) {
                call(null, e.getMessage());
            }

            @Override
            public void onIntent(byte[] bytes) {
                Intent intent = bytesToIntent(bytes);
                call(intent.getStringExtra(AccountManager.KEY_ACCOUNT_NAME), null);
            }

            private void call(String username, String error) {
                if (mPendingCallback == null) {
                    return;
                }
                mPendingCallback.call(username, error);
                mPendingCallback = null;
            }
        }, new RegisterActivityResultReceiverCallback(accountPickerIntent));
    }

    /**
     * @see AuthenticationService#clearOAuth2Token(String)
     */
    @Override
    public void clearOAuth2Token(String token) {
        try {
            GoogleAuthUtil.clearToken(mContext, token);
        } catch (GoogleAuthException | IOException e) {
            // Nothing to do.
        }
    }

    private static Intent bytesToIntent(byte[] bytes) {
        Parcel p = Parcel.obtain();
        p.unmarshall(bytes, 0, bytes.length);
        p.setDataPosition(0);
        return Intent.CREATOR.createFromParcel(p);
    }
}
