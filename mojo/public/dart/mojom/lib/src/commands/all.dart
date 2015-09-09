// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library mojom.command.all;

import 'dart:async';
import 'dart:io';

import 'package:args/args.dart';
import 'package:args/command_runner.dart';
import 'package:path/path.dart' as path;

import 'mojom_command.dart';
import '../generate.dart';
import '../utils.dart';

class AllPackagesCommand extends MojomCommand {
  String get name => 'all';
  String get description =>
      'Generate bindings for a package and all its dependencies.';
  String get invocation => 'mojom all';

  bool _fetch;
  bool _generate;
  Directory _packages;
  Directory _mojomPackage;
  List<Directory> _additionalDirs;

  AllPackagesCommand() {
    argParser.addOption('additional-mojom-dir',
        abbr: 'a',
        allowMultiple: true,
        help: 'Path to an additional directory containing mojom.dart'
            'files to put in the mojom package. May be specified multiple '
            'times.');
    argParser.addFlag('fetch',
        abbr: 'f',
        defaultsTo: false,
        negatable: true,
        help: 'Searches packages for a .mojoms file, and fetches .mojom files'
            'as speficied in that file. Implies -g.');
    argParser.addFlag('generate',
        abbr: 'g',
        defaultsTo: false,
        negatable: true,
        help: 'Generate Dart bindings for .mojom files.');
    argParser.addOption('packages-dir',
        abbr: 'p',
        defaultsTo: path.join(Directory.current.path, 'packages'),
        help: 'Path to an application\'s packages directory.');
  }

  run() async {
    await _validateArguments();

    // mojoms without a DartPackage annotation, and pregenerated mojoms from
    // [additionalDirs] will go into the mojom package, so we make a local
    // copy of it so we don't pollute the global pub cache.
    //
    // TODO(zra): Fail if a mojom has no DartPackage annotation, and remove the
    // need for [additionalDirs].
    if (!dryRun) {
      await _copyMojomPackage(_packages);
    }

    var generator = new MojomGenerator(mojoSdk,
        verbose: verbose,
        dryRun: dryRun,
        errorOnDuplicate: errorOnDuplicate,
        profile: profile);

    await generator.generateForAllPackages(
        _packages, _mojomPackage, _additionalDirs,
        fetch: _fetch, generate: _generate);

    if (profile) {
      generator.printProfile();
    }
  }

  _validateArguments() async {
    await validateArguments();

    _packages = new Directory(makeAbsolute(argResults['packages-dir']));
    if (verbose) print("packages = $_packages");
    if (!(await _packages.exists())) {
      throw new CommandLineError(
          "The packages directory $_packages must exist");
    }

    _mojomPackage = new Directory(path.join(_packages.path, 'mojom'));
    if (verbose) print("mojom package = $_mojomPackage");
    if (!(await _mojomPackage.exists())) {
      throw new CommandLineError(
          "The mojom package directory $_mojomPackage must exist");
    }

    _fetch = argResults['fetch'];
    _generate = argResults['generate'] || _fetch;

    _additionalDirs =
        await _validateAdditionalDirs(argResults['additional-mojom-dir']);
    if (verbose) print("additional_mojom_dirs = $_additionalDirs");
  }

  /// Ensures that the directories in [additionalPaths] are absolute and exist,
  /// and creates Directories for them, which are returned.
  _validateAdditionalDirs(Iterable additionalPaths) async {
    var additionalDirs = [];
    for (var mojomPath in additionalPaths) {
      final mojomDir = new Directory(makeAbsolute(mojomPath));
      if (!(await mojomDir.exists())) {
        throw new CommandLineError(
            "The additional mojom directory $mojomDir must exist");
      }
      additionalDirs.add(mojomDir);
    }
    return additionalDirs;
  }

  /// The "mojom" entry in [packages] is a symbolic link to the mojom package in
  /// the global pub cache directory. Because we might need to write package
  /// specific .mojom.dart files into the mojom package, we need to make a local
  /// copy of it.
  _copyMojomPackage(Directory packages) async {
    var link = new Link(path.join(packages.path, "mojom"));
    if (!await link.exists()) {
      // If the "mojom" entry in packages is not a symbolic link,
      // then do nothing.
      return;
    }

    var realpath = await link.resolveSymbolicLinks();
    var realDir = new Directory(realpath);
    var mojomDir = new Directory(path.join(packages.path, "mojom"));

    await link.delete();
    await mojomDir.create();
    await for (var file in realDir.list(recursive: true)) {
      if (file is File) {
        var relative = path.relative(file.path, from: realDir.path);
        var destPath = path.join(mojomDir.path, relative);
        var destDir = new Directory(path.dirname(destPath));
        await destDir.create(recursive: true);
        await file.copy(path.join(mojomDir.path, relative));
      }
    }
  }
}
