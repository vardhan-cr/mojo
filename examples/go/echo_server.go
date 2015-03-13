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

// ServiceProviderImpl implements mojo interface echo.
type EchoImpl struct{}

func (echo *EchoImpl) EchoString(inValue *string) (outValue *string, err error) {
	log.Println(*inValue)
	return inValue, nil
}

// ServiceProviderImpl implements mojo interface service provider.
type ServiceProviderImpl struct{}

func (impl *ServiceProviderImpl) ConnectToService(inInterfaceName string, inPipe system.MessagePipeHandle) error {
	if inInterfaceName != "mojo::examples::Echo" {
		inPipe.Close()
		return nil
	}
	request := echo.EchoRequest{bindings.NewMessagePipeHandleOwner(inPipe)}
	echoStub := echo.NewEchoStub(request, &EchoImpl{}, bindings.GetAsyncWaiter())
	go func() {
		for {
			if err := echoStub.ServeRequest(); err != nil {
				log.Println(err)
				echoStub.Close()
				break
			}
		}
	}()
	return nil
}

// AppImpl implements mojo interface application.
type AppImpl struct {
	shell *shell.ShellProxy
}

func (impl *AppImpl) Initialize(inShell shell.ShellPointer, inArgs *[]string, inUrl string) error {
	impl.shell = shell.NewShellProxy(inShell, bindings.GetAsyncWaiter())
	return nil
}

func (impl *AppImpl) AcceptConnection(inRequestorUrl string, inServices *service_provider.ServiceProviderRequest, inExposedServices *service_provider.ServiceProviderPointer, inResolvedUrl string) error {
	if inExposedServices != nil {
		inExposedServices.Close()
	}
	if inServices == nil {
		return nil
	}
	serviceProviderStub := service_provider.NewServiceProviderStub(*inServices, &ServiceProviderImpl{}, bindings.GetAsyncWaiter())
	go func() {
		for {
			if err := serviceProviderStub.ServeRequest(); err != nil {
				log.Println(err)
				serviceProviderStub.Close()
				break
			}
		}
	}()
	return nil
}

func (impl *AppImpl) RequestQuit() error {
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
