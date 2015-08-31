// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package main

import (
	"crypto/ecdsa"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"log"
	"net/url"
	"sync"

	"mojo/public/go/application"
	"mojo/public/go/bindings"
	"mojo/public/go/system"
	"mojo/public/interfaces/network/url_request"
	auth "mojo/services/authentication/public/interfaces/authentication"
	network "mojo/services/network/public/interfaces/network_service"
	"mojo/services/network/public/interfaces/url_loader"
	vpkg "mojo/services/vanadium/security/public/interfaces/principal"
)

//#include "mojo/public/c/system/types.h"
import "C"

const blesserURL = "https://dev.v.io/auth/google/bless"

type principalServiceImpl struct {
	app vpkg.AppInstanceName
	psd *principalServiceDelegate
}

func (pImpl *principalServiceImpl) Login() (*vpkg.Blessing, error) {
	if b := pImpl.psd.getBlessing(pImpl.app); b != nil {
		// The app has already logged in.
		return b, nil
	}

	token, err := pImpl.psd.getOAuth2Token()
	if err != nil {
		return nil, err
	}

	pub, priv, err := newPrincipalKey()
	if err != nil {
		return nil, err
	}

	wb, err := pImpl.psd.fetchWireBlessings(token, pub)
	if err != nil {
		return nil, err
	}

	pImpl.psd.addPrincipal(pImpl.app, &principal{wb, priv})
	return newBlessing(wb), nil
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
	blessing *wireBlessings
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

func (psd *principalServiceDelegate) getOAuth2Token() (string, error) {
	authReq, authPtr := auth.CreateMessagePipeForAuthenticationService()
	psd.Ctx.ConnectToApplication("mojo:authentication").ConnectToService(&authReq)
	authProxy := auth.NewAuthenticationServiceProxy(authPtr, bindings.GetAsyncWaiter())

	name, errString, _ := authProxy.SelectAccount(false /*return_last_selected*/)
	if name == nil {
		return "", fmt.Errorf("Failed to select an account for user:%s", errString)
	}

	token, errString, _ := authProxy.GetOAuth2Token(*name, []string{"email"})
	if token == nil {
		return "", fmt.Errorf("Failed to obtain OAuth2 token for selected account:%s", errString)
	}
	return *token, nil
}

func (psd *principalServiceDelegate) fetchWireBlessings(token string, pub publicKey) (*wireBlessings, error) {
	networkReq, networkPtr := network.CreateMessagePipeForNetworkService()
	psd.Ctx.ConnectToApplication("mojo:network_service").ConnectToService(&networkReq)
	networkProxy := network.NewNetworkServiceProxy(networkPtr, bindings.GetAsyncWaiter())

	urlLoaderReq, urlLoaderPtr := url_loader.CreateMessagePipeForUrlLoader()
	if err := networkProxy.CreateUrlLoader(urlLoaderReq); err != nil {
		return nil, fmt.Errorf("Failed to create url loader: %v", err)
	}
	urlLoaderProxy := url_loader.NewUrlLoaderProxy(urlLoaderPtr, bindings.GetAsyncWaiter())

	req, err := blessingRequestURL(token, pub)
	if err != nil {
		return nil, err
	}

	resp, err := urlLoaderProxy.Start(*req)
	if err != nil || resp.Error != nil {
		return nil, fmt.Errorf("Blessings request to Vanadium Identity Provider failed: %v(%v)", err, resp.Error)
	}

	res, b := (*resp.Body).ReadData(system.MOJO_READ_DATA_FLAG_ALL_OR_NONE)
	if res != system.MOJO_RESULT_OK {
		return nil, fmt.Errorf("Failed to read response (blessings) from Vanadium Identity Provider. Result: %v", res)
	}

	var wb wireBlessings
	if err := json.Unmarshal(b, &wb); err != nil {
		return nil, fmt.Errorf("Failed to unmarshal response (blessings) from Vanadium Identity Provider: %v", err)
	}
	// TODO(ataly, gauthamt): We should verify all signatures on the certificate chains in the
	// wire blessings to ensure that it was not tampered with.
	return &wb, nil
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
		return newBlessing(p.blessing)
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

func blessingRequestURL(token string, pub publicKey) (*url_request.UrlRequest, error) {
	baseURL, err := url.Parse(blesserURL)
	if err != nil {
		return nil, err
	}

	pubBytes, err := pub.MarshalBinary()
	if err != nil {
		return nil, err
	}

	params := url.Values{}
	params.Add("public_key", base64.URLEncoding.EncodeToString(pubBytes))
	params.Add("token", token)
	params.Add("output_format", "json")

	baseURL.RawQuery = params.Encode()
	return &url_request.UrlRequest{Url: baseURL.String(), Method: "GET"}, nil
}

//export MojoMain
func MojoMain(handle C.MojoHandle) C.MojoResult {
	application.Run(&principalServiceDelegate{}, system.MojoHandle(handle))
	return C.MOJO_RESULT_OK
}

func main() {
}
