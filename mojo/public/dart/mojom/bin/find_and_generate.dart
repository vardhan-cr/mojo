// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/// This script finds package subdirectories containing a .mojoms file and
/// invokes Mojo bindings generation for those packages.

import 'dart:async';
import 'dart:io';

import 'package:args/args.dart' as args;
import 'package:mojom/generate.dart';
import 'package:path/path.dart' as path;

main(List<String> arguments) async {
  var parser = new args.ArgParser()
    ..addOption('root-dir',
        abbr: 'r',
        defaultsTo: Directory.current.path,
        help: 'Directory from which to begin the search.')
    ..addOption('mojo-sdk',
        abbr: 'm',
        defaultsTo: Platform.environment['MOJO_SDK'],
        help: 'Absolute path to the Mojo SDK, which can also be specified '
        'with the environment variable MOJO_SDK.')
    ..addFlag('profile', abbr: 'p', defaultsTo: false)
    ..addFlag('verbose', abbr: 'v', defaultsTo: false);
  var result = parser.parse(arguments);
  var mojoSdk = makeAbsolute(result['mojo-sdk']);
  var rootDir = makeAbsolute(result['root-dir']);
  var treeGenerator = new TreeGenerator(mojoSdk, rootDir,
      verbose: result['verbose'], profile: result['profile']);
  await treeGenerator.findAndGenerate();
  return treeGenerator.errors;
}
