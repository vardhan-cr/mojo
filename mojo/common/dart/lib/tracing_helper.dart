// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library tracing;

import 'trace_provider_impl.dart';

import 'dart:async';
import 'dart:convert';
import 'dart:core';
import 'dart:io';
import 'dart:isolate';

import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';
import 'package:mojo_services/tracing/tracing.mojom.dart';

// TracingHelper is used by Dart code running in the Mojo shell in order
// to perform tracing.
class TracingHelper {
  TraceProviderImpl _impl;
  int _tid;

  // Construct an instance of TracingHelper from within your application's
  // |initialize()| method. |appName| will be used to form a thread identifier
  // for use in trace messages. If |appName| is longer than 20 characters then
  // only the last 20 characters of |appName| will be used.
  TracingHelper(Application app, String appName) {
    // We use only the last 20 characters of appName to form the tid so that
    // the 9-digit Isolate hash code we are appending won't get truncated by the
    // tracing UI.
    if (appName.length > 20) {
      appName = appName.substring(appName.length - 20);
    }
    _tid = appName.hashCode ^ Isolate.current.hashCode;

    _impl = new TraceProviderImpl();
    ApplicationConnection connection = app.connectToApplication("mojo:tracing");
    connection.provideService(TraceProviderName, (e) {
      _impl.connect(e);
    });
  }

  // Invoke this at the beginning of a function whose duration you wish to
  // trace. Invoke |end()| on the returned object.
  FunctionTrace beginFunction(String functionName, String categories,
      {Map<String, String> args}) {
    assert(functionName != null);
    _sendTraceMessage(functionName, categories, "B", args: args);
    return new _FunctionTraceImpl(this, functionName, categories);
  }

  void _endFunction(String functionName, String categories) {
    _sendTraceMessage(functionName, categories, "E");
  }

  void traceInstant(String name, String categories,
      {Map<String, String> args}) {
    _sendTraceMessage(name, categories, "I", args: args);
  }

  void _sendTraceMessage(String name, String categories, String phase,
      {Map<String, String> args}) {
    var map = {};
    map["name"] = name;
    map["cat"] = categories;
    map["ph"] = phase;
    map["ts"] = getTimeTicksNow();
    map["pid"] = pid;
    map["tid"] = _tid;
    if (args != null) {
      map["args"] = args;
    }
    _impl.sendTraceMessage(JSON.encode(map));
  }

  // A convenience method that wraps a closure in a begin-end pair of
  // tracing calls.
  dynamic trace(String functionName, String categories, closure(),
      {Map<String, String> args}) {
    FunctionTrace ft = beginFunction(functionName, categories, args: args);
    final returnValue = closure();
    ft.end();
    return returnValue;
  }
}

// A an instance of FunctionTrace is returned from |beginFunction()|.
// Invoke |end()| from every exit point in the function you are tracing.
abstract class FunctionTrace {
  void end();
}

class _FunctionTraceImpl implements FunctionTrace {
  TracingHelper _tracing;
  String _functionName;
  String _categories;

  _FunctionTraceImpl(this._tracing, this._functionName, this._categories);

  @override
  void end() {
    if (_functionName != null) {
      _tracing._endFunction(_functionName, _categories);
    }
  }
}
