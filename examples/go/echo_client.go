// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package main

import (
	"fmt"
	"log"

	"code.google.com/p/go.mobile/app"

	"mojo/public/go/bindings"
	"mojo/public/go/system"

	"examples/echo/echo"
	"mojo/public/interfaces/application/application"
	"mojo/public/interfaces/application/service_provider"
	"mojo/public/interfaces/application/shell"
)

//#include "mojo/public/c/system/types.h"
import "C"

// AppImpl implements mojo interface application.
type AppImpl struct {
	shell           *shell.ShellProxy
	serviceProvider *service_provider.ServiceProviderProxy
	echo            *echo.EchoProxy
}

func (impl *AppImpl) Initialize(inShell shell.ShellPointer, inArgs *[]string, inUrl string) error {
	impl.shell = shell.NewShellProxy(inShell, bindings.GetAsyncWaiter())
	// Connect to another mojo application.
	request, pointer := service_provider.CreateMessagePipeForServiceProvider()
	if err := impl.shell.ConnectToApplication("mojo:echo_server", &request, nil); err != nil {
		return err
	}
	impl.serviceProvider = service_provider.NewServiceProviderProxy(pointer, bindings.GetAsyncWaiter())
	// Connect to service.
	echoRequest, echoPointer := echo.CreateMessagePipeForEcho()
	if err := impl.serviceProvider.ConnectToService("mojo::examples::Echo", echoRequest.PassMessagePipe()); err != nil {
		return err
	}
	impl.echo = echo.NewEchoProxy(echoPointer, bindings.GetAsyncWaiter())
	// Send and receive echo request.
	response, err := impl.echo.EchoString(bindings.StringPointer("Hello Go world!"))
	if response != nil {
		fmt.Println(*response)
	} else {
		return fmt.Errorf("nil echo response")
	}
	return err
}

func (impl *AppImpl) AcceptConnection(inRequestorUrl string, inServices *service_provider.ServiceProviderRequest, inExposedServices *service_provider.ServiceProviderPointer, inResolvedUrl string) error {
	if inServices != nil {
		inServices.Close()
	}
	if inExposedServices != nil {
		inExposedServices.Close()
	}
	return nil
}

func (impl *AppImpl) RequestQuit() error {
	impl.echo.Close_proxy()
	impl.serviceProvider.Close_proxy()
	impl.shell.Close_proxy()
	return fmt.Errorf("closed")
}

//export MojoMain
func MojoMain(handle C.MojoHandle) C.MojoResult {
	appHandle := system.GetCore().AcquireNativeHandle(system.MojoHandle(handle)).ToMessagePipeHandle()
	appRequest := application.ApplicationRequest{bindings.NewMessagePipeHandleOwner(appHandle)}
	stub := application.NewApplicationStub(appRequest, &AppImpl{}, bindings.GetAsyncWaiter())
	for {
		if err := stub.ServeRequest(); err != nil {
			log.Println(err)
			stub.Close()
			break
		}
	}
	return C.MOJO_RESULT_OK
}

func main() {
	app.Run(app.Callbacks{})
}
