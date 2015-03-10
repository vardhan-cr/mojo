// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.services.location;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.location.LocationServices;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ApplicationRunner;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojom.mojo.LocationService;
import org.chromium.mojom.mojo.Shell;

/**
 * Android service application implementing the LocationService interface using Google play
 * services API.
 */
class LocationServiceApp implements ApplicationDelegate {
    private static String TAG = "LocationServiceApp";

    private final GoogleApiClient mGoogleApiClient;
    private final Core mCore;

    /**
     * Android looper thread class to get callbacks from google play services api.
     */
    private static class LooperThread extends Thread {
        private Handler mHandler;
        private Looper mLooper;

        @Override
        public void run() {
            synchronized (this) {
                Looper.prepare();
                mHandler = new Handler();
                mLooper = Looper.myLooper();
                this.notify();
            }
            Looper.loop();
        }

        public Looper getLooper() {
            synchronized (this) {
                while (mLooper == null) {
                    try {
                        this.wait();
                    } catch (InterruptedException e) {
                        Log.e(TAG, e.getMessage(), e);
                    }
                }
            }
            return mLooper;
        }

        public void quitSoon() {
            synchronized (this) {
                while (mLooper == null) {
                    try {
                        this.wait();
                    } catch (InterruptedException e) {
                        Log.e(TAG, e.getMessage(), e);
                    }
                }
            }
            mLooper.quitSafely();
        }
    }
    private final LooperThread mLooperThread = new LooperThread();

    public LocationServiceApp(Context context, Core core) {
        // TODO(alhaad): Create a test mode which uses mock locations instead of real locations.
        mGoogleApiClient =
                new GoogleApiClient.Builder(context).addApi(LocationServices.API).build();
        mCore = core;
    }

    /**
     * @see ApplicationDelegate#initialize(Shell, String[], String)
     */
    @Override
    public void initialize(Shell shell, String[] args, String url) {
        ConnectionResult connectionResult = mGoogleApiClient.blockingConnect();
        if (!connectionResult.isSuccess()) {
            Log.e(TAG, "Could not connect to Google play services. ConnectionResult: "
                            + connectionResult.toString());
            mCore.getCurrentRunLoop().quit();
            return;
        }
        mLooperThread.start();
    }

    /**
     * @see ApplicationDelegate#configureIncomingConnection(String, ApplicationConnection)
     */
    @Override
    public boolean configureIncomingConnection(
            final String requestorUrl, ApplicationConnection connection) {
        connection.addService(new ServiceFactoryBinder<LocationService>() {
            @Override
            public void bindNewInstanceToMessagePipe(MessagePipeHandle pipe) {
                LocationService.MANAGER.bind(
                        new LocationServiceImpl(mGoogleApiClient, mLooperThread.getLooper(),
                                mCore.getCurrentRunLoop()),
                        pipe);
            }

            @Override
            public String getInterfaceName() {
                return LocationService.MANAGER.getName();
            }
        });
        return true;
    }

    /**
     * @see ApplicationDelegate#quit()
     */
    @Override
    public void quit() {
        mGoogleApiClient.disconnect();
        mLooperThread.quitSoon();
        try {
            mLooperThread.join();
        } catch (InterruptedException e) {
            Log.e(TAG, e.getMessage(), e);
        }
    }

    public static void mojoMain(
            Context context, Core core, MessagePipeHandle applicationRequestHandle) {
        ApplicationRunner.run(
                new LocationServiceApp(context, core), core, applicationRequestHandle);
    }
}
