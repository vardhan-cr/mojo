// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// To run: mojo/tools/mojo_shell.py --sky \
//             examples/dart/speech_input/speech_input.dart --android
//
// The speech_recognizer service is only available for Android targets.

import 'package:mojom/speech_recognizer/speech_recognizer.mojom.dart';
import 'package:sky/mojo/embedder.dart';

import 'package:sky/widgets/raised_button.dart';
import 'package:sky/widgets/basic.dart';

class ListenApp extends App {
  ListenApp() {
    embedder.connectToService(
        "mojo:speech_recognizer", _speechRecognizerService);
  }
  Widget build() {
    List<Text> result_widgets = new List();
    if (_result != null) {
      if (_result.tag == ResultOrErrorTag.results) {
        for (var candidate in _result.results) {
          result_widgets.add(new Text(candidate.text));
        }
      } else {
        result_widgets.add(new Text("ERROR: ${_result.errorCode}"));
      }
    }
    return new Container(
        padding: new EdgeDims.all(10.0),
        margin: new EdgeDims.all(10.0),
        decoration: new BoxDecoration(
            backgroundColor: const Color(0xFFCCCCCCCC)),
        child: new Flex([
      new Block(result_widgets),
      new RaisedButton(
          child: new Text('LISTEN'),
          enabled: !_listening,
          onPressed: () => listen())
    ],
        direction: FlexDirection.vertical,
        justifyContent: FlexJustifyContent.spaceBetween));
  }

  void listen() {
    print("listening...");
    setState(() {
      _listening = true;
    });

    var future = _speechRecognizerService.ptr.listen();
    future.then((response) {
      setState(() {
        _listening = false;
        _result = response.resultOrError;
      });
    });
  }

  SpeechRecognizerServiceProxy _speechRecognizerService =
      new SpeechRecognizerServiceProxy.unbound();

  var _result = null;

  bool _listening = false;
}

void main() {
  runApp(new ListenApp());
}
