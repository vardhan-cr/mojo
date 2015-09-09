// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library mojom.command.tree;

import 'dart:async';
import 'dart:io';

import 'package:args/args.dart';
import 'package:args/command_runner.dart';
import 'package:path/path.dart' as path;

import 'mojom_command.dart';
import '../generate.dart';
import '../utils.dart';

class TreeCommand extends MojomCommand {
  String get name => 'tree';
  String get description =>
      'Generate bindings for all Dart packages in a source tree.';
  String get invocation => 'mojom tree';

  Directory _root;
  List<String> _skips;

  TreeCommand() {
    argParser.addOption('root',
        abbr: 'r',
        defaultsTo: Directory.current.path,
        help: 'Directory from which to begin the search for Dart packages.');
    argParser.addOption('skip',
        abbr: 's', allowMultiple: true, help: 'Directories to skip.');
  }

  run() async {
    await _validateArguments();
    var treeGenerator = new TreeGenerator(mojoSdk, _root, _skips,
        verbose: verbose, profile: profile, dryRun: dryRun);
    await treeGenerator.findAndGenerate();
    return treeGenerator.errors;
  }

  _validateArguments() async {
    await validateArguments();

    _root = new Directory(makeAbsolute(argResults['root']));
    _skips = argResults['skip'].map(makeAbsolute).toList();
  }
}
