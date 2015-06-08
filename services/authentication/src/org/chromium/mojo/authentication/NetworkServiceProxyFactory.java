// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.authentication;

import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.application.ShellHelper;
import org.chromium.mojo.bindings.Connector;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojom.mojo.NetworkService;
import org.chromium.mojom.mojo.Shell;

/**
 * Service factory for the network service proxy that requests the real network service, configures
 * it for
 * authentication and then forwards the calls from the client to the real network service.
 * <p>
 * TODO(qsr): Handle authentication. At the moment, this class is only forwarding calls.
 */
public class NetworkServiceProxyFactory implements ServiceFactoryBinder<NetworkService> {
    private final Core mCore;
    private final Shell mShell;

    public NetworkServiceProxyFactory(Core core, Shell shell) {
        mCore = core;
        mShell = shell;
    }

    /**
     * @see ServiceFactoryBinder#bind(InterfaceRequest)
     */
    @Override
    public void bind(InterfaceRequest<NetworkService> request) {
        NetworkService.Proxy ns = ShellHelper.connectToService(
                mCore, mShell, "mojo:network_service", NetworkService.MANAGER);
        forwardHandles(ns.getProxyHandler().passHandle(), request.passHandle());
    }

    /**
     * @see ServiceFactoryBinder#getInterfaceName()
     */
    @Override
    public String getInterfaceName() {
        return NetworkService.MANAGER.getName();
    }

    /**
     * Forwards messages from h1 to h2 and back.
     */
    private void forwardHandles(MessagePipeHandle h1, MessagePipeHandle h2) {
        Connector c1 = new Connector(h1);
        Connector c2 = new Connector(h2);
        c1.setIncomingMessageReceiver(c2);
        c2.setIncomingMessageReceiver(c1);
        c1.start();
        c2.start();
    }
}
