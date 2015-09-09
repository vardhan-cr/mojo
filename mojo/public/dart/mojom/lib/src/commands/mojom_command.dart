// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library mojom.command;

import 'dart:async';
import 'dart:io';

import 'package:args/args.dart';
import 'package:args/command_runner.dart';
import 'package:path/path.dart' as path;

import '../utils.dart';

abstract class MojomCommand extends Command {
  bool _verbose;
  bool _dryRun;
  bool _errorOnDuplicate;
  bool _profile;
  Directory _mojoSdk;

  bool get verbose => _verbose;
  bool get dryRun => _dryRun;
  bool get errorOnDuplicate => _errorOnDuplicate;
  bool get profile => _profile;
  Directory get mojoSdk => _mojoSdk;

  validateArguments() async {
    _verbose = globalResults['verbose'];
    _dryRun = globalResults['dry-run'];
    _errorOnDuplicate = !globalResults['ignore-duplicates'];
    _profile = globalResults['profile'];

    final mojoSdkPath = globalResults['mojo-sdk'];
    if (mojoSdkPath == null) {
      throw new CommandLineError(
          "The Mojo SDK directory must be specified with the --mojo-sdk "
          "flag or the MOJO_SDK environment variable.");
    }
    _mojoSdk = new Directory(makeAbsolute(mojoSdkPath));
    if (_verbose) print("Mojo SDK = $_mojoSdk");
    if (!(await _mojoSdk.exists())) {
      throw new CommandLineError(
          "The specified Mojo SDK directory $_mojoSdk must exist.");
    }
  }
}
