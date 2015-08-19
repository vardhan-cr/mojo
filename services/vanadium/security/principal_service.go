// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package main

import (
	"fmt"
	"log"
	"sync"

	"mojo/public/go/application"
	"mojo/public/go/bindings"
	"mojo/public/go/system"
	auth "mojo/services/authentication/public/interfaces/authentication"
	"mojo/services/vanadium/security/public/interfaces/principal"
)

//#include "mojo/public/c/system/types.h"
import "C"

type PrincipalServiceImpl struct {
	app principal.AppInstanceName
	psd *PrincipalServiceDelegate
}

func (pImpl *PrincipalServiceImpl) Login() (b *principal.Blessing, err error) {
	authReq, authPtr := auth.CreateMessagePipeForAuthenticationService()
	pImpl.psd.Ctx.ConnectToApplication("mojo:authentication").ConnectToService(&authReq)
	authProxy := auth.NewAuthenticationServiceProxy(authPtr, bindings.GetAsyncWaiter())
	name, errString, _ := authProxy.SelectAccount(false /*return_last_selected*/)
	if name != nil {
		cert := []principal.Certificate{principal.Certificate{Extension: *name}}
		b = &principal.Blessing{cert}
		pImpl.psd.addUserBlessing(pImpl.app, b)
	} else {
		err = fmt.Errorf("Failed to authenticate user:%s", errString)
	}
	return
}

func (pImpl *PrincipalServiceImpl) Logout() (err error) {
	pImpl.psd.deleteUserBlessing(pImpl.app)
	return
}

func (pImpl *PrincipalServiceImpl) GetUserBlessing(app principal.AppInstanceName) (*principal.Blessing, error) {
	return pImpl.psd.getUserBlessing(app), nil
}

func (pImpl *PrincipalServiceImpl) Create(req principal.PrincipalService_Request) {
	stub := principal.NewPrincipalServiceStub(req, pImpl, bindings.GetAsyncWaiter())
	pImpl.psd.addStubForCleanup(stub)
	go func() {
		for {
			if err := stub.ServeRequest(); err != nil {
				connectionError, ok := err.(*bindings.ConnectionError)
				if !ok || !connectionError.Closed() {
					log.Println(err)
				}
				break
			}
		}
	}()
}

type PrincipalServiceDelegate struct {
	bMap  map[principal.AppInstanceName]*principal.Blessing
	Ctx   application.Context
	mu    sync.Mutex
	stubs []*bindings.Stub
}

func (psd *PrincipalServiceDelegate) Initialize(context application.Context) {
	psd.bMap = make(map[principal.AppInstanceName]*principal.Blessing)
	psd.Ctx = context
}

func (psd *PrincipalServiceDelegate) AcceptConnection(connection *application.Connection) {
	app := principal.AppInstanceName{
		Url:       connection.RequestorURL(),
		Qualifier: nil,
	}
	connection.ProvideServices(&principal.PrincipalService_ServiceFactory{&PrincipalServiceImpl{app, psd}})
}

func (psd *PrincipalServiceDelegate) addStubForCleanup(stub *bindings.Stub) {
	psd.mu.Lock()
	defer psd.mu.Unlock()
	psd.stubs = append(psd.stubs, stub)
}

func (psd *PrincipalServiceDelegate) addUserBlessing(app principal.AppInstanceName, b *principal.Blessing) {
	psd.mu.Lock()
	defer psd.mu.Unlock()
	psd.bMap[app] = b
}

func (psd *PrincipalServiceDelegate) getUserBlessing(app principal.AppInstanceName) *principal.Blessing {
	psd.mu.Lock()
	defer psd.mu.Unlock()
	return psd.bMap[app]
}

func (psd *PrincipalServiceDelegate) deleteUserBlessing(app principal.AppInstanceName) {
	psd.mu.Lock()
	defer psd.mu.Unlock()
	delete(psd.bMap, app)
}

func (psd *PrincipalServiceDelegate) Quit() {
	psd.mu.Lock()
	defer psd.mu.Unlock()
	for _, stub := range psd.stubs {
		stub.Close()
	}
}

//export MojoMain
func MojoMain(handle C.MojoHandle) C.MojoResult {
	application.Run(&PrincipalServiceDelegate{}, system.MojoHandle(handle))
	return C.MOJO_RESULT_OK
}

func main() {
}
