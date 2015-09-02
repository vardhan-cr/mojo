// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

part of generate;

class GenerateOptions {
  final Directory packages;
  final Directory mojoSdk;
  final Directory mojomPackage;
  final Directory singlePackage;
  final List<Directory> additionalDirs;
  final bool fetch;
  final bool generate;
  final bool errorOnDuplicate;
  final bool verbose;
  final bool dryRun;
  final bool profile;
  GenerateOptions(this.packages, this.mojoSdk, this.mojomPackage,
      this.singlePackage, this.additionalDirs, this.fetch, this.generate,
      this.errorOnDuplicate, this.verbose, this.dryRun, this.profile);
}

String makeAbsolute(String p) => path.isAbsolute(p)
                               ? path.normalize(p)
                               : path.normalize(path.absolute(p));

/// Ensures that the directories in [additionalPaths] are absolute and exist,
/// and creates Directories for them, which are returned.
Future<List<Directory>> validateAdditionalDirs(Iterable additionalPaths) async {
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

Future<GenerateOptions> parseArguments(List<String> arguments) async {
  final parser = new args.ArgParser()
    ..addOption('additional-mojom-dir',
        abbr: 'a',
        allowMultiple: true,
        help: 'Path to an additional directory containing mojom.dart'
        'files to put in the mojom package. May be specified multiple times.')
    ..addFlag('dry-run',
        abbr: 'd',
        defaultsTo: false,
        help: 'Print the operations that would have been run, but'
        'do not run anything.')
    ..addFlag('fetch',
        abbr: 'f',
        defaultsTo: false,
        help: 'Searches packages for a .mojoms file, and fetches .mojom files'
        'as speficied in that file. Implies -g.')
    ..addFlag('generate',
        abbr: 'g',
        defaultsTo: false,
        help: 'Generate Dart bindings for .mojom files.')
    ..addFlag('ignore-duplicates',
        abbr: 'i',
        defaultsTo: false,
        help: 'Ignore generation of a .mojom.dart file into the same location '
        'as an existing file. By default this is an error')
    ..addOption('mojo-sdk',
        abbr: 'm',
        defaultsTo: Platform.environment['MOJO_SDK'],
        help: 'Path to the Mojo SDK, which can also be specified '
        'with the environment variable MOJO_SDK.')
    ..addOption('package-root',
        abbr: 'p',
        defaultsTo: path.join(Directory.current.path, 'packages'),
        help: 'Path to an application\'s package root.')
    ..addFlag('profile',
        abbr: 'r',
        defaultsTo: false,
        help: 'Display some profiling information on exit.')
    ..addOption('single-package',
        abbr: 's',
        defaultsTo: null,
        help: 'Instead of traversing the packages directory and generating '
        'bindings. Generate bindings only for the package at the given '
        'path.')
    ..addFlag('verbose', abbr: 'v', defaultsTo: false);
  final result = parser.parse(arguments);
  bool verbose = result['verbose'];
  bool dryRun = result['dry-run'];
  bool errorOnDuplicate = !result['ignore-duplicates'];
  bool profile = result['profile'];

  Directory singlePackage;
  if (result['single-package'] != null) {

    singlePackage = new Directory(makeAbsolute(result['single-package']));
    if (!(await singlePackage.exists())) {
      throw new CommandLineError(
          "$singlePackage for --single-pacakge does not exist");
    }
  }

  var packages;
  var mojomPackage;
  if (singlePackage == null) {
    packages = new Directory(makeAbsolute(result['package-root']));
    if (verbose) print("packages = $packages");
    if (!(await packages.exists())) {
      throw new CommandLineError("The packages directory $packages must exist");
    }

    mojomPackage = new Directory(path.join(packages.path, 'mojom'));
    if (verbose) print("mojom package = $mojomPackage");
    if (!(await mojomPackage.exists())) {
      throw new CommandLineError(
          "The mojom package directory $mojomPackage must exist");
    }
  }

  final fetch = result['fetch'];
  final generate = result['generate'] || fetch || (singlePackage != null);
  var mojoSdk = null;
  if (generate) {
    final mojoSdkPath = result['mojo-sdk'];
    if (mojoSdkPath == null) {
      throw new CommandLineError(
          "The Mojo SDK directory must be specified with the --mojo-sdk flag or"
          "the MOJO_SDK environment variable.");
    }
    mojoSdk = new Directory(makeAbsolute(mojoSdkPath));
    if (verbose) print("Mojo SDK = $mojoSdk");
    if (!(await mojoSdk.exists())) {
      throw new CommandLineError(
          "The specified Mojo SDK directory $mojoSdk must exist.");
    }
  }

  final additionalDirs =
      await validateAdditionalDirs(result['additional-mojom-dir']);
  if (verbose) print("additional_mojom_dirs = $additionalDirs");

  return new GenerateOptions(packages, mojoSdk, mojomPackage, singlePackage,
      additionalDirs, fetch, generate, errorOnDuplicate, verbose, dryRun,
      profile);
}
