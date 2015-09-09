// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library mojom.command.single;

import 'dart:async';
import 'dart:io';

import 'package:args/args.dart';
import 'package:args/command_runner.dart';
import 'package:path/path.dart' as path;

import 'mojom_command.dart';
import '../generate.dart';
import '../utils.dart';

class SinglePackageCommand extends MojomCommand {
  String get name => 'single';
  String get description => 'Generate bindings for a package.';
  String get invocation => 'mojom single';

  Directory _package;

  SinglePackageCommand() {
    argParser.addOption('package',
        abbr: 'p',
        defaultsTo: Directory.current.path,
        help: 'Path to a Dart package.');
  }

  run() async {
    await _validateArguments();

    var generator = new MojomGenerator(mojoSdk,
        verbose: verbose,
        dryRun: dryRun,
        errorOnDuplicate: errorOnDuplicate,
        profile: profile);
    await generator.generateForPackage(_package);
    if (profile) {
      generator.printProfile();
    }
  }

  _validateArguments() async {
    await validateArguments();

    _package = new Directory(makeAbsolute(argResults['package']));
    if (verbose) print("package = $_package");
    if (!(await _package.exists())) {
      throw new CommandLineError("The package $_package must exist");
    }
  }
}
