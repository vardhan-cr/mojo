// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library mojom.command_runner;

import 'dart:async';
import 'dart:io';

import 'package:args/args.dart';
import 'package:args/command_runner.dart';

import 'commands/all.dart';
import 'commands/check.dart';
import 'commands/single.dart';
import 'commands/tree.dart';

class MojomCommandRunner extends CommandRunner {
  MojomCommandRunner()
      : super("mojom", "mojom is a tool for managing Mojo bindings for Dart.") {
    super.argParser.addFlag('dry-run',
        abbr: 'd',
        defaultsTo: false,
        negatable: false,
        help: 'Print the operations that would have been run, but'
            'do not run anything.');
    super.argParser.addFlag('ignore-duplicates',
        abbr: 'i',
        defaultsTo: false,
        negatable: false,
        help: 'Ignore generation of a .mojom.dart file into the same location '
            'as an existing file. By default this is an error');
    super.argParser.addOption('mojo-sdk',
        abbr: 'm',
        defaultsTo: Platform.environment['MOJO_SDK'],
        help: 'Path to the Mojo SDK, which can also be specified '
            'with the environment variable MOJO_SDK.');
    super.argParser.addFlag('profile',
        abbr: 'p',
        defaultsTo: false,
        negatable: false,
        help: 'Display some profiling information on exit.');
    super.argParser.addFlag('verbose',
        abbr: 'v',
        defaultsTo: false,
        negatable: false,
        help: 'Show extra output about what mojom is doing.');

    super.addCommand(new AllPackagesCommand());
    super.addCommand(new SinglePackageCommand());
    super.addCommand(new TreeCheckCommand());
    super.addCommand(new TreeCommand());
  }
}
