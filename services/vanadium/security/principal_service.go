// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package main

import (
	"crypto/ecdsa"
	"fmt"
	"log"
	"sync"

	"mojo/public/go/application"
	"mojo/public/go/bindings"
	"mojo/public/go/system"
	auth "mojo/services/authentication/public/interfaces/authentication"
	vpkg "mojo/services/vanadium/security/public/interfaces/principal"
)

//#include "mojo/public/c/system/types.h"
import "C"

type principalServiceImpl struct {
	app vpkg.AppInstanceName
	psd *principalServiceDelegate
}

func newPrincipal(name string) (*vpkg.Certificate, *ecdsa.PrivateKey, error) {
	pubKey, privKey, err := newPrincipalKey()
	if err != nil {
		return nil, nil, err
	}
	var pubKeyBytes []byte
	if pubKeyBytes, err = pubKey.MarshalBinary(); err != nil {
		return nil, nil, err
	}
	return &vpkg.Certificate{name, &pubKeyBytes}, privKey, nil
}

func (pImpl *principalServiceImpl) Login() (b *vpkg.Blessing, err error) {
	authReq, authPtr := auth.CreateMessagePipeForAuthenticationService()
	pImpl.psd.Ctx.ConnectToApplication("mojo:authentication").ConnectToService(&authReq)
	authProxy := auth.NewAuthenticationServiceProxy(authPtr, bindings.GetAsyncWaiter())
	name, errString, _ := authProxy.SelectAccount(false /*return_last_selected*/)
	if name != nil {
		cert, privKey, err := newPrincipal(*name)
		if err != nil {
			return nil, err
		}
		b = &vpkg.Blessing{[]vpkg.Certificate{*cert}}
		pImpl.psd.addPrincipal(pImpl.app, &principal{b, privKey})
	} else {
		err = fmt.Errorf("Failed to authenticate user:%s", errString)
	}
	return
}

func (pImpl *principalServiceImpl) Logout() (err error) {
	pImpl.psd.deletePrincipal(pImpl.app)
	return
}

func (pImpl *principalServiceImpl) GetUserBlessing(app vpkg.AppInstanceName) (*vpkg.Blessing, error) {
	return pImpl.psd.getBlessing(app), nil
}

func (pImpl *principalServiceImpl) Create(req vpkg.PrincipalService_Request) {
	stub := vpkg.NewPrincipalServiceStub(req, pImpl, bindings.GetAsyncWaiter())
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

type principal struct {
	blessing *vpkg.Blessing
	private  *ecdsa.PrivateKey
}

type principalServiceDelegate struct {
	table map[vpkg.AppInstanceName]*principal
	Ctx   application.Context
	mu    sync.Mutex
	stubs []*bindings.Stub
}

func (psd *principalServiceDelegate) Initialize(context application.Context) {
	psd.table = make(map[vpkg.AppInstanceName]*principal)
	psd.Ctx = context
}

func (psd *principalServiceDelegate) AcceptConnection(connection *application.Connection) {
	app := vpkg.AppInstanceName{
		Url:       connection.RequestorURL(),
		Qualifier: nil,
	}
	connection.ProvideServices(&vpkg.PrincipalService_ServiceFactory{&principalServiceImpl{app, psd}})
}

func (psd *principalServiceDelegate) addStubForCleanup(stub *bindings.Stub) {
	psd.mu.Lock()
	defer psd.mu.Unlock()
	psd.stubs = append(psd.stubs, stub)
}

func (psd *principalServiceDelegate) addPrincipal(app vpkg.AppInstanceName, p *principal) {
	psd.mu.Lock()
	defer psd.mu.Unlock()
	psd.table[app] = p
}

func (psd *principalServiceDelegate) getBlessing(app vpkg.AppInstanceName) *vpkg.Blessing {
	psd.mu.Lock()
	defer psd.mu.Unlock()
	if p, ok := psd.table[app]; ok {
		return p.blessing
	}
	return nil
}

func (psd *principalServiceDelegate) deletePrincipal(app vpkg.AppInstanceName) {
	psd.mu.Lock()
	defer psd.mu.Unlock()
	delete(psd.table, app)
}

func (psd *principalServiceDelegate) Quit() {
	psd.mu.Lock()
	defer psd.mu.Unlock()
	for _, stub := range psd.stubs {
		stub.Close()
	}
}

//export MojoMain
func MojoMain(handle C.MojoHandle) C.MojoResult {
	application.Run(&principalServiceDelegate{}, system.MojoHandle(handle))
	return C.MOJO_RESULT_OK
}

func main() {
}
