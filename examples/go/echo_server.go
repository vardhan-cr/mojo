// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package main

import (
	"log"

	"code.google.com/p/go.mobile/app"

	"mojo/public/go/application"
	"mojo/public/go/bindings"
	"mojo/public/go/system"

	"examples/echo/echo"
)

//#include "mojo/public/c/system/types.h"
import "C"

type EchoImpl struct{}

func (echo *EchoImpl) EchoString(inValue *string) (outValue *string, err error) {
	log.Println(*inValue)
	return inValue, nil
}

type EchoServerDelegate struct {
	stubs []*bindings.Stub
}

func (delegate *EchoServerDelegate) Initialize(context application.Context) {}

func (delegate *EchoServerDelegate) Create(request echo.EchoRequest) {
	stub := echo.NewEchoStub(request, &EchoImpl{}, bindings.GetAsyncWaiter())
	delegate.stubs = append(delegate.stubs, stub)
	go func() {
		for {
			if err := stub.ServeRequest(); err != nil {
				// TODO(rogulenko): don't log in case message pipe was closed
				log.Println(err)
				break
			}
		}
	}()
}

func (delegate *EchoServerDelegate) AcceptConnection(connection *application.Connection) {
	connection.ProvideServices(&echo.EchoServiceFactory{delegate})
}

func (delegate *EchoServerDelegate) Quit() {
	for _, stub := range delegate.stubs {
		stub.Close()
	}
}

//export MojoMain
func MojoMain(handle C.MojoHandle) C.MojoResult {
	application.Run(&EchoServerDelegate{}, system.MojoHandle(handle))
	return C.MOJO_RESULT_OK
}

func main() {
	app.Run(app.Callbacks{})
}
