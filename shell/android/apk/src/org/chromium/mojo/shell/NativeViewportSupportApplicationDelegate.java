// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.content.Context;
import android.content.Intent;

import org.chromium.base.ApplicationStatus;
import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojom.mojo.Shell;
import org.chromium.mojom.native_viewport.NativeViewportShellService;

final class NativeViewportSupportApplicationDelegate implements ApplicationDelegate {
    private class NativeViewportShellServiceFactoryBinder
            implements ServiceFactoryBinder<NativeViewportShellService> {
        @Override
        public void bind(InterfaceRequest<NativeViewportShellService> request) {
            NativeViewportShellService.MANAGER.bind(new NativeViewportShellServiceImpl(), request);
        }

        @Override
        public String getInterfaceName() {
            return NativeViewportShellService.MANAGER.getName();
        }
    }

    private class NativeViewportShellServiceImpl implements NativeViewportShellService {
        @Override
        public void close() {}

        @Override
        public void onConnectionError(MojoException e) {}

        @Override
        public void createNewNativeWindow() {
            Context context = ApplicationStatus.getApplicationContext();
            Intent newDocumentIntent = new Intent(context, ViewportActivity.class);
            newDocumentIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
            newDocumentIntent.addFlags(Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
            newDocumentIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

            context.startActivity(newDocumentIntent);
        }
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
        if (!connection.getRequestorUrl().equals("mojo://native_viewport_service/")) {
            return false;
        }
        connection.addService(new NativeViewportShellServiceFactoryBinder());
        return true;
    }

    /**
     * @see ApplicationDelegate#quit()
     */
    @Override
    public void quit() {}
}
