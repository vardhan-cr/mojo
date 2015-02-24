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

	"gen/mojom/mojo/test"
	"mojo/public/go/bindings"
	"mojo/public/go/system"
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

func pipeToRequest(h system.MessagePipeHandle) bindings.InterfaceRequest {
	return bindings.InterfaceRequest{bindings.NewMessagePipeHandleOwner(h)}
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

func TestConformanceValidation(t *testing.T) {
	tests := getMatchingTests(listTestFiles(), "conformance_")
	waiter := bindings.GetAsyncWaiter()

	h := NewMockMessagePipeHandle()
	proxyIn, proxyOut := h, h
	interfacePointer := test.ConformanceTestInterfacePointer(pipeToRequest(proxyIn))
	impl := &conformanceValidator{false, test.NewConformanceTestInterfaceProxy(interfacePointer, waiter)}

	h = NewMockMessagePipeHandle()
	stubIn, stubOut := h, h
	interfaceRequest := test.ConformanceTestInterfaceRequest(pipeToRequest(stubOut))
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

type integrationStubImpl struct {
}

func (s *integrationStubImpl) Method0(inParam0 test.BasicStruct) ([]uint8, error) {
	return nil, nil
}

func runIntegrationTest(t *testing.T, tests []string, testRequest, testResponse bool) {
	waiter := bindings.GetAsyncWaiter()
	_, stubIn, stubOut := core.CreateMessagePipe(nil)
	interfaceRequest := test.IntegrationTestInterfaceRequest(pipeToRequest(stubOut))
	stub := test.NewIntegrationTestInterfaceStub(interfaceRequest, &integrationStubImpl{}, waiter)
	_, proxyIn, proxyOut := core.CreateMessagePipe(nil)
	interfacePointer := test.IntegrationTestInterfacePointer(pipeToRequest(proxyIn))
	proxy := test.NewIntegrationTestInterfaceProxy(interfacePointer, waiter)

	inArg := test.BasicStruct{}
	for i, test := range tests {
		bytes, handles := readTest(test)
		answer := readAnswer(test)
		// Replace request ID to match proxy numbers.
		bytes[16] = byte(i + 1)

		if testRequest {
			stubIn.WriteMessage(bytes, handles, system.MOJO_WRITE_MESSAGE_FLAG_NONE)
			err := stub.ServeRequest()
			if (err == nil) != (answer == "PASS") {
				t.Fatalf("unexpected result for test %v: %v", test, err)
			}
		}
		if testResponse {
			proxyOut.WriteMessage(bytes, handles, system.MOJO_WRITE_MESSAGE_FLAG_NONE)
			_, err := proxy.Method0(inArg)
			if (err == nil) != (answer == "PASS") {
				t.Fatalf("unexpected result for test %v: %v", test, err)
			}
		}
	}
	stubIn.Close()
	stubOut.Close()
	proxyIn.Close()
	proxyOut.Close()
}

func TestIntegrationRequestValidation(t *testing.T) {
	tests := getMatchingTests(listTestFiles(), "integration_intf_rqst")
	runIntegrationTest(t, tests, true, false)
}

func TestIntegrationResponseValidation(t *testing.T) {
	tests := getMatchingTests(listTestFiles(), "integration_intf_resp")
	runIntegrationTest(t, tests, false, true)
}

func TestIntegrationHeaderValidation(t *testing.T) {
	tests := getMatchingTests(listTestFiles(), "integration_msghdr")
	runIntegrationTest(t, tests, true, true)
}
