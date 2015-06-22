// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ApplicationRunner;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.mojom.mojo.Shell;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Utility class that allows to register java application bundled with the shell.
 * <p>
 * To add a new application, add your registerService() call in the create() method of this class
 * with the URL of the application to register and then ServiceFactoryBinder for the registered
 * service.
 */
@JNINamespace("shell")
public class JavaApplicationRegistry {
    private final Map<String, List<ServiceFactoryBinder<?>>> mServiceFactoryBinder =
            new HashMap<>();

    private static final class ApplicationRunnable implements ApplicationDelegate, Runnable {
        private final List<ServiceFactoryBinder<?>> mBinders;
        private final MessagePipeHandle mApplicationRequestHandle;

        private ApplicationRunnable(
                List<ServiceFactoryBinder<?>> binders, MessagePipeHandle applicationRequestHandle) {
            mBinders = binders;
            mApplicationRequestHandle = applicationRequestHandle;
        }

        /**
         * @see ApplicationDelegate#initialize(Shell, String[], String)
         */
        @Override
        public void initialize(Shell shell, String[] args, String url) {}

        /**
         * @see ApplicationDelegate#configureIncomingConnection(ApplicationConnection)
         */
        @Override
        public boolean configureIncomingConnection(ApplicationConnection connection) {
            for (ServiceFactoryBinder<?> binder : mBinders) {
                connection.addService(binder);
            }
            return true;
        }

        /**
         * @see ApplicationDelegate#quit()
         */
        @Override
        public void quit() {}

        /**
         * @see Runnable#run()
         */
        @Override
        public void run() {
            ApplicationRunner.run(this, CoreImpl.getInstance(), mApplicationRequestHandle);
        }
    }

    private JavaApplicationRegistry() {}

    private void registerService(String url, ServiceFactoryBinder<?> serviceFactoryBinder) {
        List<ServiceFactoryBinder<?>> binders = mServiceFactoryBinder.get(url);
        if (binders == null) {
            binders = new ArrayList<>();
            mServiceFactoryBinder.put(url, binders);
        }
        binders.add(serviceFactoryBinder);
    }

    @CalledByNative
    private String[] getApplications() {
        return mServiceFactoryBinder.keySet().toArray(new String[mServiceFactoryBinder.size()]);
    }

    @CalledByNative
    private void startApplication(String url, int applicationRequestHandle) {
        MessagePipeHandle messagePipeHandle = CoreImpl.getInstance()
                                                      .acquireNativeHandle(applicationRequestHandle)
                                                      .toMessagePipeHandle();
        List<ServiceFactoryBinder<?>> binders = mServiceFactoryBinder.get(url);
        if (binders != null) {
            new Thread(new ApplicationRunnable(binders, messagePipeHandle)).start();
        } else {
            messagePipeHandle.close();
        }
    }

    @CalledByNative
    private static JavaApplicationRegistry create() {
        JavaApplicationRegistry registry = new JavaApplicationRegistry();
        // Register services.
        registry.registerService("mojo:keyboard", new KeyboardFactory());
        registry.registerService("mojo:intent_receiver", IntentReceiverRegistry.getInstance());
        return registry;
    }
}
