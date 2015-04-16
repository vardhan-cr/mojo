// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package tests

import (
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"reflect"
	"strings"
	"testing"

	"mojo/public/go/bindings"
	"mojo/public/go/system"
	test "mojo/public/interfaces/bindings/tests/validation_test_interfaces"
)

func getTestPath(name string) string {
	// TODO(rogulenko): try to get a better solution.
	// This should be .../out/name{Debug|Release}/obj/mojo/go.
	dir, err := filepath.Abs(filepath.Dir(os.Args[0]))
	if err != nil {
		panic(err)
	}
	// Go 5 folders up.
	for i := 0; i < 5; i++ {
		dir = filepath.Dir(dir)
	}
	testsFolder := filepath.Join("mojo", "public", "interfaces", "bindings", "tests", "data", "validation")
	if name != "" {
		return filepath.Join(dir, testsFolder, name)
	} else {
		return filepath.Join(dir, testsFolder)
	}
}

func listTestFiles() []string {
	files, err := ioutil.ReadDir(getTestPath(""))
	if err != nil {
		panic(err)
	}
	var fileNames []string
	for _, file := range files {
		if file.Mode().IsRegular() {
			fileNames = append(fileNames, file.Name())
		}
	}
	return fileNames
}

func getMatchingTests(fileNames []string, prefix string) []string {
	var result []string
	extension := ".data"
	for _, fileName := range fileNames {
		if strings.HasPrefix(fileName, prefix) && strings.HasSuffix(fileName, extension) {
			result = append(result, strings.TrimSuffix(fileName, extension))
		}
	}
	if len(result) == 0 {
		panic("empty test list")
	}
	return result
}

func readTest(testName string) ([]byte, []system.UntypedHandle) {
	content, err := ioutil.ReadFile(getTestPath(testName + ".data"))
	if err != nil {
		panic(err)
	}
	lines := strings.Split(strings.Replace(string(content), "\r", "\n", -1), "\n")
	for i, _ := range lines {
		lines[i] = strings.Split(lines[i], "//")[0]
	}
	parser := &inputParser{}
	bytes, handles := parser.Parse(strings.Join(lines, " "))
	return bytes, handles
}

func readAnswer(testName string) string {
	content, err := ioutil.ReadFile(getTestPath(testName + ".expected"))
	if err != nil {
		panic(err)
	}
	return strings.TrimSpace(string(content))
}

func pipeOwner(h system.MessagePipeHandle) bindings.MessagePipeHandleOwner {
	return bindings.NewMessagePipeHandleOwner(h)
}

type rawMessage struct {
	Bytes   []byte
	Handles []system.UntypedHandle
}

type mockMessagePipeHandle struct {
	bindings.InvalidHandle
	messages chan rawMessage
}

func NewMockMessagePipeHandle() system.MessagePipeHandle {
	h := &mockMessagePipeHandle{}
	h.messages = make(chan rawMessage, 10)
	return h
}

func (h *mockMessagePipeHandle) IsValid() bool {
	return true
}

func (h *mockMessagePipeHandle) ToUntypedHandle() system.UntypedHandle {
	return h
}

func (h *mockMessagePipeHandle) ToMessagePipeHandle() system.MessagePipeHandle {
	return h
}

func (h *mockMessagePipeHandle) ReadMessage(flags system.MojoReadMessageFlags) (system.MojoResult, []byte, []system.UntypedHandle) {
	message := <-h.messages
	return system.MOJO_RESULT_OK, message.Bytes, message.Handles
}

func (h *mockMessagePipeHandle) WriteMessage(bytes []byte, handles []system.UntypedHandle, flags system.MojoWriteMessageFlags) system.MojoResult {
	h.messages <- rawMessage{bytes, handles}
	return system.MOJO_RESULT_OK
}

type conformanceValidator struct {
	CheckArgs bool
	Proxy     test.ConformanceTestInterface
}

func (v *conformanceValidator) Method0(inParam0 float32) error {
	if !v.CheckArgs {
		return nil
	}
	param0 := float32(-1)
	if inParam0 != param0 {
		return fmt.Errorf("unexpected value (Method0, inParam0): expected %v, got %v", param0, inParam0)
	}
	return v.Proxy.Method0(inParam0)
}

func (v *conformanceValidator) Method1(inParam0 test.StructA) error {
	if !v.CheckArgs {
		return nil
	}
	param0 := test.StructA{1234}
	if !reflect.DeepEqual(inParam0, param0) {
		return fmt.Errorf("unexpected value (Method1, inParam0): expected %v, got %v", param0, inParam0)
	}
	return v.Proxy.Method1(inParam0)
}

func (v *conformanceValidator) Method2(inParam0 test.StructB, inParam1 test.StructA) error {
	if !v.CheckArgs {
		return nil
	}
	param0 := test.StructB{test.StructA{12345}}
	param1 := test.StructA{67890}
	if !reflect.DeepEqual(inParam0, param0) {
		return fmt.Errorf("unexpected value (Method2, inParam0): expected %v, got %v", param0, inParam0)
	}
	if !reflect.DeepEqual(inParam1, param1) {
		return fmt.Errorf("unexpected value (Method2, inParam1): expected %v, got %v", param1, inParam1)
	}
	return v.Proxy.Method2(inParam0, inParam1)
}

func (v *conformanceValidator) Method3(inParam0 []bool) error {
	if !v.CheckArgs {
		return nil
	}
	param0 := []bool{true, false, true, false, true, false, true, false, true, true, true, true}
	if !reflect.DeepEqual(inParam0, param0) {
		return fmt.Errorf("unexpected value (Method3, inParam0): expected %v, got %v", param0, inParam0)
	}
	return v.Proxy.Method3(inParam0)
}

func (v *conformanceValidator) Method4(inParam0 test.StructC, inParam1 []uint8) error {
	if !v.CheckArgs {
		return nil
	}
	param0 := test.StructC{[]uint8{0, 1, 2}}
	param1 := []uint8{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
	if !reflect.DeepEqual(inParam0, param0) {
		return fmt.Errorf("unexpected value (Method4, inParam0): expected %v, got %v", param0, inParam0)
	}
	if !reflect.DeepEqual(inParam1, param1) {
		return fmt.Errorf("unexpected value (Method4, inParam1): expected %v, got %v", param1, inParam1)
	}
	return v.Proxy.Method4(inParam0, inParam1)
}

func (v *conformanceValidator) Method5(inParam0 test.StructE, inParam1 system.ProducerHandle) error {
	if !v.CheckArgs {
		return nil
	}
	param0 := test.StructE{
		test.StructD{[]system.MessagePipeHandle{
			&mockHandle{handle: 1},
			&mockHandle{handle: 2},
		}},
		&mockHandle{handle: 4},
	}
	param1 := &mockHandle{handle: 5}
	if !reflect.DeepEqual(inParam0, param0) {
		return fmt.Errorf("unexpected value (Method5, inParam0): expected %v, got %v", param0, inParam0)
	}
	if !reflect.DeepEqual(inParam1, param1) {
		return fmt.Errorf("unexpected value (Method5, inParam1): expected %v, got %v", param1, inParam1)
	}
	return v.Proxy.Method5(inParam0, inParam1)
}

func (v *conformanceValidator) Method6(inParam0 [][]uint8) error {
	if !v.CheckArgs {
		return nil
	}
	param0 := [][]uint8{[]uint8{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}}
	if !reflect.DeepEqual(inParam0, param0) {
		return fmt.Errorf("unexpected value (Method6, inParam0): expected %v, got %v", param0, inParam0)
	}
	return v.Proxy.Method6(inParam0)
}

func (v *conformanceValidator) Method7(inParam0 test.StructF, inParam1 [2]*[3]uint8) error {
	if !v.CheckArgs {
		return nil
	}
	param0 := test.StructF{[3]uint8{0, 1, 2}}
	param1 := [2]*[3]uint8{
		nil,
		&[3]uint8{0, 1, 2},
	}
	if !reflect.DeepEqual(inParam0, param0) {
		return fmt.Errorf("unexpected value (Method7, inParam0): expected %v, got %v", param0, inParam0)
	}
	if !reflect.DeepEqual(inParam1, param1) {
		return fmt.Errorf("unexpected value (Method7, inParam1): expected %v, got %v", param1, inParam1)
	}
	return v.Proxy.Method7(inParam0, inParam1)
}

func (v *conformanceValidator) Method8(inParam0 []*[]string) error {
	if !v.CheckArgs {
		return nil
	}
	param0 := []*[]string{
		nil,
		&[]string{string([]byte{0, 1, 2, 3, 4})},
		nil,
	}
	if !reflect.DeepEqual(inParam0, param0) {
		return fmt.Errorf("unexpected value (Method8, inParam0): expected %v, got %v", param0, inParam0)
	}
	return v.Proxy.Method8(inParam0)
}

func (v *conformanceValidator) Method9(inParam0 *[][]*system.Handle) error {
	if !v.CheckArgs {
		return nil
	}
	handles := []system.Handle{
		&mockHandle{handle: 1},
		&mockHandle{handle: 3},
		&mockHandle{handle: 4},
	}
	param0 := &[][]*system.Handle{
		[]*system.Handle{&handles[0], nil},
		[]*system.Handle{&handles[1], nil, &handles[2]},
	}
	if !reflect.DeepEqual(inParam0, param0) {
		return fmt.Errorf("unexpected value (Method9, inParam0): expected %v, got %v", param0, inParam0)
	}
	return v.Proxy.Method9(inParam0)
}

func (v *conformanceValidator) Method10(inParam0 map[string]uint8) error {
	if !v.CheckArgs {
		return nil
	}
	param0 := map[string]uint8{
		string([]byte{0, 1, 2, 3, 4}): 1,
		string([]byte{5, 6, 7, 8, 9}): 2,
	}
	if !reflect.DeepEqual(inParam0, param0) {
		return fmt.Errorf("unexpected value (Method10, inParam0): expected %v, got %v", param0, inParam0)
	}
	return v.Proxy.Method10(inParam0)
}

func (v *conformanceValidator) Method11(test.StructG) error {
	return nil
}

func (v *conformanceValidator) Method12(float32) (float32, error) {
	return 0, nil
}

func (v *conformanceValidator) Method13(*test.InterfaceAPointer, uint32, *test.InterfaceAPointer) error {
	return nil
}

func TestConformanceValidation(t *testing.T) {
	tests := getMatchingTests(listTestFiles(), "conformance_")

	h := NewMockMessagePipeHandle()
	proxyIn, proxyOut := h, h
	interfacePointer := test.ConformanceTestInterfacePointer{pipeOwner(proxyIn)}
	impl := &conformanceValidator{false, test.NewConformanceTestInterfaceProxy(interfacePointer, waiter)}

	h = NewMockMessagePipeHandle()
	stubIn, stubOut := h, h
	interfaceRequest := test.ConformanceTestInterfaceRequest{pipeOwner(stubOut)}
	stub := test.NewConformanceTestInterfaceStub(interfaceRequest, impl, waiter)
	for _, test := range tests {
		bytes, handles := readTest(test)
		answer := readAnswer(test)
		impl.CheckArgs = strings.HasSuffix(test, "_good")
		stubIn.WriteMessage(bytes, handles, system.MOJO_WRITE_MESSAGE_FLAG_NONE)
		err := stub.ServeRequest()
		if (err == nil) != (answer == "PASS") {
			t.Fatalf("unexpected result for test %v: %v", test, err)
		}

		if !impl.CheckArgs {
			continue
		}
		// Decode again to verify correctness of encoding.
		_, bytes, handles = proxyOut.ReadMessage(system.MOJO_READ_MESSAGE_FLAG_NONE)
		stubIn.WriteMessage(bytes, handles, system.MOJO_WRITE_MESSAGE_FLAG_NONE)
		if err := stub.ServeRequest(); err != nil {
			t.Fatalf("error processing encoded data for test %v: %v", test, err)
		}
		proxyOut.ReadMessage(system.MOJO_READ_MESSAGE_FLAG_NONE)
		// Do not compare encoded bytes, as some tests contain maps, that
		// can be encoded randomly.
	}
}

func runTests(t *testing.T, prefix string, in system.MessagePipeHandle, check func() error) {
	tests := getMatchingTests(listTestFiles(), prefix)
	for i, test := range tests {
		bytes, handles := readTest(test)
		answer := readAnswer(test)
		// Replace request ID to match proxy numbers.
		if bytes[0] == 24 {
			bytes[16] = byte(i + 1)
		}
		in.WriteMessage(bytes, handles, system.MOJO_WRITE_MESSAGE_FLAG_NONE)
		err := check()
		if (err == nil) != (answer == "PASS") {
			t.Fatalf("unexpected result for test %v: %v", test, err)
		}
	}
}

type integrationStubImpl struct{}

func (s *integrationStubImpl) Method0(test.BasicStruct) ([]uint8, error) {
	return nil, nil
}

func TestIntegrationTests(t *testing.T) {
	_, stubIn, stubOut := core.CreateMessagePipe(nil)
	interfaceRequest := test.IntegrationTestInterfaceRequest{pipeOwner(stubOut)}
	stub := test.NewIntegrationTestInterfaceStub(interfaceRequest, &integrationStubImpl{}, waiter)
	checkStub := func() error { return stub.ServeRequest() }
	runTests(t, "integration_intf_rqst_", stubIn, checkStub)
	runTests(t, "integration_msghdr_", stubIn, checkStub)
	stubIn.Close()
	stub.Close()

	_, proxyIn, proxyOut := core.CreateMessagePipe(nil)
	interfacePointer := test.IntegrationTestInterfacePointer{pipeOwner(proxyIn)}
	proxy := test.NewIntegrationTestInterfaceProxy(interfacePointer, waiter)
	checkProxy := func() error {
		_, err := proxy.Method0(test.BasicStruct{})
		return err
	}
	runTests(t, "integration_intf_resp_", proxyOut, checkProxy)
	runTests(t, "integration_msghdr_", proxyOut, checkProxy)
	proxy.Close_proxy()
	proxyOut.Close()
}

type boundsCheckStubImpl struct{}

func (s *boundsCheckStubImpl) Method0(uint8) (uint8, error) {
	return 0, nil
}

func (s *boundsCheckStubImpl) Method1(uint8) error {
	return nil
}

func TestBoundsCheck(t *testing.T) {
	_, stubIn, stubOut := core.CreateMessagePipe(nil)
	interfaceRequest := test.BoundsCheckTestInterfaceRequest{pipeOwner(stubOut)}
	stub := test.NewBoundsCheckTestInterfaceStub(interfaceRequest, &boundsCheckStubImpl{}, waiter)
	checkStub := func() error { return stub.ServeRequest() }
	runTests(t, "boundscheck_", stubIn, checkStub)
	stubIn.Close()
	stub.Close()

	_, proxyIn, proxyOut := core.CreateMessagePipe(nil)
	interfacePointer := test.BoundsCheckTestInterfacePointer{pipeOwner(proxyIn)}
	proxy := test.NewBoundsCheckTestInterfaceProxy(interfacePointer, waiter)
	checkProxy := func() error {
		_, err := proxy.Method0(0)
		return err
	}
	runTests(t, "resp_boundscheck_", proxyOut, checkProxy)
	proxy.Close_proxy()
	proxyOut.Close()
}

func TestConformanceResponse(t *testing.T) {
	_, proxyIn, proxyOut := core.CreateMessagePipe(nil)
	interfacePointer := test.ConformanceTestInterfacePointer{pipeOwner(proxyIn)}
	proxy := test.NewConformanceTestInterfaceProxy(interfacePointer, waiter)
	checkProxy := func() error {
		_, err := proxy.Method12(0)
		return err
	}
	runTests(t, "resp_conformance_", proxyOut, checkProxy)
	proxy.Close_proxy()
	proxyOut.Close()
}
