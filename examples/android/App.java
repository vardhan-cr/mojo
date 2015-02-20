// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.examples.android;

import android.content.Context;
import android.util.Log;

import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.RunLoop;
import org.chromium.mojom.mojo.Application;
import org.chromium.mojom.mojo.ExampleService;
import org.chromium.mojom.mojo.ServiceProvider;
import org.chromium.mojom.mojo.Shell;

import java.util.HashMap;
import java.util.Map;

/**
 * App holds all the code for this example application.
 */
class App {
    private static String TAG = "ExampleServiceApp";

    public static class ApplicationImpl implements Application {
        private final Core mCore;
        private Shell mShell;

        public ApplicationImpl(Core core, MessagePipeHandle applicationRequest) {
            ApplicationImpl.MANAGER.bind(this, applicationRequest);
            mCore = core;
        }

        @Override
        public void initialize(Shell shell, String[] args, String url) {
            mShell = shell;
        }

        @Override
        public void acceptConnection(String requestorUrl,
                InterfaceRequest<ServiceProvider> services, ServiceProvider exposedServices) {
            ServiceProviderImpl serviceProvider = new ServiceProviderImpl();
            serviceProvider.addService(
                    new ServiceFactoryBinder<ExampleService>() {
                        @Override
                        public void bindNewInstanceToMessagePipe(
                                MessagePipeHandle pipe) {
                            ExampleService.MANAGER.bind(new ExampleServiceImpl(), pipe);
                        }

                        @Override
                        public String getInterfaceName() {
                            return ExampleService.MANAGER.getName();
                        }
                    }
            );
            ServiceProvider.MANAGER.bind(serviceProvider, services);
            exposedServices.close();
        };

        @Override
        public void requestQuit() {
            mCore.getCurrentRunLoop().quit();
        }

        @Override
        public void close() {
            if (mShell != null) {
                mShell.close();
            }
        }

        @Override
        public void onConnectionError(MojoException e) {}
    }

    /**
     * ServiceFactoryBinder holds the necessary information to bind a service interface to a
     * message pipe.
     */
    private static interface ServiceFactoryBinder<T extends Interface> {
        public void bindNewInstanceToMessagePipe(MessagePipeHandle pipe);
        public String getInterfaceName();
    }

    public static class ServiceProviderImpl implements ServiceProvider {
        private Map<String, ServiceFactoryBinder<? extends Interface>> mNameToServiceMap =
                new HashMap<String, ServiceFactoryBinder<? extends Interface>>();

        public ServiceProviderImpl() {}

        /**
         * AddService adds a new service to the list of services provided by this application.
         */
        public void addService(ServiceFactoryBinder<? extends Interface> binder) {
            mNameToServiceMap.put(binder.getInterfaceName(), binder);
        }

        @Override
        public void connectToService(String interfaceName, MessagePipeHandle pipe) {
            if (mNameToServiceMap.containsKey(interfaceName)) {
                mNameToServiceMap.get(interfaceName).bindNewInstanceToMessagePipe(pipe);
            } else {
                pipe.close();
            }
        }

        @Override
        public void close() {}

        @Override
        public void onConnectionError(MojoException e) {}
    }

    public static class ExampleServiceImpl implements ExampleService {
        public ExampleServiceImpl() {}

        @Override
        public void ping(short pingValue, ExampleService.PingResponse callback) {
            callback.call(pingValue);
        }

        @Override
        public void close() {}

        @Override
        public void onConnectionError(MojoException e) {}
    }

    public static void mojoMain(Context context, Core core,
            MessagePipeHandle applicationRequestHandle) {
        Log.i(TAG, "App.MojoMain called");
        try (RunLoop runLoop = core.createDefaultRunLoop()) {
            Log.i(TAG, "Run loop created");

            Application application = new ApplicationImpl(core, applicationRequestHandle);

            Log.i(TAG, "Setup done, starting run loop");
            runLoop.run();
            Log.i(TAG, "Run loop exited");
        }
    }
}
