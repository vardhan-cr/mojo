// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/// This library generates Mojo bindings for a Dart package.

library generate;

import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:path/path.dart' as path;

import 'utils.dart';

class _GenerateIterData extends SubDirIterData {
  final Directory _packageRoot;
  _GenerateIterData(this._packageRoot) : super(null);
  Directory get packageRoot => _packageRoot;
}

class MojomGenerator {
  Map<String, String> _duplicateDetection;
  bool _errorOnDuplicate;
  bool _verbose;
  bool _dryRun;
  bool _profile;
  Directory _mojoSdk;
  int _generationMs;

  MojomGenerator(this._mojoSdk,
      {bool errorOnDuplicate: true,
      bool verbose: false,
      bool profile: false,
      bool dryRun: false})
      : _errorOnDuplicate = errorOnDuplicate,
        _verbose = verbose,
        _dryRun = dryRun,
        _profile = profile,
        _generationMs = 0,
        _duplicateDetection = new Map<String, String>();

  /// Generate bindings for [pacakge]. Fail if there is no .mojoms file, or if
  /// the .mojoms file refers to .mojom files lacking a DartPackage annotation
  /// for [pacakge]
  generateForPackage(Directory package) async {
    Directory temp = await package.createTemp();
    Directory packageRoot = new Directory(path.join(package.path, 'packages'));
    File mojomsFile = new File(path.join(package.path, '.mojoms'));
    if (!(await mojomsFile.exists())) {
      await temp.delete(recursive: true);
      throw new FetchError(
          "--single-package specified but no .mojoms file found: $mojomsFile");
    }
    DateTime dotMojomsTime = (await mojomsFile.stat()).modified;
    DateTime mojomTime =
        await _fetchFromDotMojoms(package.uri, temp, mojomsFile);
    DateTime mojomDartTime = await _findOldestMojomDart(package);

    if (_shouldRegenerate(dotMojomsTime, mojomTime, mojomDartTime)) {
      await for (var dir in temp.list()) {
        if (dir is! Directory) continue;
        var mojomDir = new Directory(path.join(dir.path, 'mojom'));
        if (_verbose) print("pathSegments = ${package.uri.pathSegments}");
        await _generateForMojomDir(mojomDir, packageRoot,
            packageName: package.uri.pathSegments.lastWhere((s) => s != ""));
      }
    }
    await temp.delete(recursive: true);
  }

  generateForAllPackages(Directory packageRoot, Directory mojomPackage,
      List<Directory> additionalDirs,
      {bool fetch: false, bool generate: false}) async {
    // Fetch .mojom files. These will be picked up by the generation step
    // below.
    if (fetch) {
      await subDirIter(packageRoot, null, _fetchAction);
    }

    // Generate mojom files.
    if (generate) {
      await _generateDirIter(packageRoot, new _GenerateIterData(packageRoot));
    }

    // TODO(zra): As mentioned above, this should go away.
    // Copy pregenerated files from specified external directories into the
    // mojom package.
    final data = new _GenerateIterData(packageRoot);
    data.subdir = mojomPackage;
    for (var mojomDir in additionalDirs) {
      await _copyBindingsAction(data, mojomDir);
      if (generate) {
        await _generateAction(data, mojomDir);
      }
    }
  }

  void printProfile() {
    print("Generation time: $_generationMs ms");
  }

  /// Under [package]/lib, returns the oldest modification time for a
  /// .mojom.dart file.
  _findOldestMojomDart(Directory package) async {
    DateTime oldestModTime;
    Directory libDir = new Directory(path.join(package.path, 'lib'));
    if (!await libDir.exists()) return null;
    await for (var file in libDir.list(recursive: true)) {
      if (file is! File) continue;
      if (!isMojomDart(file.path)) continue;
      DateTime modTime = (await file.stat()).modified;
      if ((oldestModTime == null) || oldestModTime.isAfter(modTime)) {
        oldestModTime = modTime;
      }
    }
    return oldestModTime;
  }

  /// If the .mojoms file or the newest .mojom is newer than the oldest
  /// .mojom.dart, then regenerate everything.
  bool _shouldRegenerate(
      DateTime dotMojomsTime, DateTime mojomTime, DateTime mojomDartTime) {
    return (dotMojomsTime == null) ||
        (mojomTime == null) ||
        (mojomDartTime == null) ||
        dotMojomsTime.isAfter(mojomDartTime) ||
        mojomTime.isAfter(mojomDartTime);
  }

  /// Given a .mojoms file in |mojomsFile|, fetch the listed .mojom files and
  /// store them in a directory tree rooted at |destination|. Relative file URIs
  /// are resolved relative to |base|. The "mojom" directories populated under
  /// |destination| are suitable for passing to |generateForMojomDir| above.
  ///
  /// The .mojoms file should be formatted as follows:
  /// '''
  /// root: https://www.example.com/mojoms
  /// path/to/some/mojom1.mojom
  /// path/to/some/other/mojom2.mojom
  ///
  /// root: https://www.example-two.com/mojoms
  /// path/to/example/two/mojom1.mojom
  /// ...
  ///
  /// root: file:///absolute/path/to/mojoms
  /// ...
  ///
  /// root /some/absolute/path
  /// ...
  ///
  /// root: ../../path/relative/to/|base|/mojoms
  /// ...
  ///
  /// Lines beginning with '#' are ignored.
  ///
  /// Returns the modification time of the newest .mojom found.
  _fetchFromDotMojoms(Uri base, Directory destination, File mojomsFile) async {
    DateTime newestModTime;
    Directory mojomsDir;
    var httpClient = new HttpClient();
    int repoCount = 0;
    int mojomCount = 0;
    Uri repoRoot;
    for (String line in await mojomsFile.readAsLines()) {
      line = line.trim();
      if (line.isEmpty || line.startsWith('#')) continue;

      if (line.startsWith('root:')) {
        if ((mojomsDir != null) && (mojomCount == 0)) {
          throw new FetchError("root with no mojoms: $repoRoot");
        }
        mojomCount = 0;
        var rootWords = line.split(" ");
        if (rootWords.length != 2) {
          throw new FetchError("Malformed root: $line");
        }
        repoRoot = Uri.parse(rootWords[1]);
        String scheme = repoRoot.scheme;
        if (!scheme.startsWith("http")) {
          // If not an absolute path. resolve relative to packageUri.
          if (!repoRoot.isAbsolute) {
            repoRoot = base.resolveUri(repoRoot);
          }
        }
        if (_verbose) print("Found repo root: $repoRoot");

        mojomsDir = new Directory(
            path.join(destination.path, 'mojm.repo.$repoCount', 'mojom'));
        await mojomsDir.create(recursive: true);
        repoCount++;
      } else {
        if (mojomsDir == null) {
          throw new FetchError('Malformed .mojoms file: $mojomsFile');
        }
        Uri uri = repoRoot.resolve(line);
        DateTime modTime = await getModificationTime(uri);
        if (_verbose) print("Fetching $uri");
        String fileString = await fetchUri(httpClient, uri);
        String filePath = path.join(mojomsDir.path, line);
        var file = new File(filePath);
        if (!await file.exists()) {
          await file.create(recursive: true);
          await file.writeAsString(fileString);
          if (_verbose) print("Wrote $filePath");
        }
        if ((newestModTime == null) || newestModTime.isBefore(modTime)) {
          newestModTime = modTime;
        }
        mojomCount++;
      }
    }
    return newestModTime;
  }

  /// Generate bindings for .mojom files found in [source].
  /// Bindings will be generated into [destination]/$package/... where
  /// $package is the package specified by the bindings generation process.
  /// If [packageName] is given, an exception is thrown if
  /// $package != [packageName]
  _generateForMojomDir(Directory source, Directory destination,
      {String packageName}) async {
    await for (var mojom in source.list(recursive: true)) {
      if (mojom is! File) continue;
      if (!isMojom(mojom.path)) continue;
      if (_verbose) print("Found $mojom");

      final script = path.join(
          _mojoSdk.path, 'tools', 'bindings', 'mojom_bindings_generator.py');
      final sdkInc = path.normalize(path.join(_mojoSdk.path, '..', '..'));
      final outputDir = await destination.createTemp();
      final output = outputDir.path;
      final arguments = [
        '--use_bundled_pylibs',
        '-g',
        'dart',
        '-o',
        output,
        // TODO(zra): Are other include paths needed?
        '-I',
        sdkInc,
        '-I',
        source.path,
        mojom.path
      ];

      if (_verbose || _dryRun) {
        print('Generating $mojom');
        print('$script ${arguments.join(" ")}');
      }
      if (!_dryRun) {
        var stopwatch;
        if (_profile) stopwatch = new Stopwatch()..start();
        final result = await Process.run(script, arguments);
        if (_profile) {
          stopwatch.stop();
          _generationMs += stopwatch.elapsedMilliseconds;
        }
        if (result.exitCode != 0) {
          await outputDir.delete(recursive: true);
          throw new GenerationError("$script failed:\n${result.stderr}");
        }

        // Generated .mojom.dart is under $output/dart-pkg/$PACKAGE/lib/$X
        // Move $X to |destination|/$PACKAGE/$X
        final generatedDirName = path.join(output, 'dart-pkg');
        final generatedDir = new Directory(generatedDirName);
        await for (var genpack in generatedDir.list()) {
          if (genpack is! Directory) continue;
          var libDir = new Directory(path.join(genpack.path, 'lib'));
          var name = path.relative(genpack.path, from: generatedDirName);

          if ((packageName != null) && packageName != name) {
            await outputDir.delete(recursive: true);
            throw new GenerationError(
                "Tried to generate for package $name in package $packageName");
          }

          var copyDest = new Directory(path.join(destination.path, name));
          var copyData = new SubDirIterData(copyDest);
          if (_verbose) print("Copy $libDir to $copyDest");
          await _copyBindingsAction(copyData, libDir);
        }

        await outputDir.delete(recursive: true);
      }
    }
  }

  /// Searches for .mojom.dart files under [sourceDir] and copies them to
  /// [data.subdir].
  _copyBindingsAction(SubDirIterData data, Directory sourceDir) async {
    await for (var mojom in sourceDir.list(recursive: true)) {
      if (mojom is! File) continue;
      if (!isMojomDart(mojom.path)) continue;
      if (_verbose) print("Found $mojom");

      final relative = path.relative(mojom.path, from: sourceDir.path);
      final dest = path.join(data.subdir.path, relative);
      final destDirectory = new Directory(path.dirname(dest));

      if (_errorOnDuplicate && _duplicateDetection.containsKey(dest)) {
        String original = _duplicateDetection[dest];
        throw new GenerationError(
            'Conflict: Both ${original} and ${mojom.path} supply ${dest}');
      }
      _duplicateDetection[dest] = mojom.path;

      if (_verbose || _dryRun) {
        print('Copying $mojom to $dest');
      }

      if (!_dryRun) {
        final File source = new File(mojom.path);
        final File destFile = new File(dest);
        if (await destFile.exists()) {
          await destFile.delete();
        }
        if (_verbose) print("Ensuring $destDirectory exists");
        await destDirectory.create(recursive: true);
        await source.copy(dest);
        await markFileReadOnly(dest);
      }
    }
  }

  /// Searches for .mojom files under [mojomDirectory], generates .mojom.dart
  /// files for them, and copies them to the 'mojom' package.
  _generateAction(_GenerateIterData data, Directory mojomDirectory) async {
    if (_verbose) print(
        "generateAction: $mojomDirectory, packageRoot: ${data.packageRoot}");
    await _generateForMojomDir(mojomDirectory, data.packageRoot);
  }

  /// Iterates recurisvley over all subdirectories called "mojom" under
  /// |packages| and applies bindings generation to them, placing the results
  /// in the right place under |packages|.
  _generateDirIter(Directory packages, _GenerateIterData data) async {
    await subDirIter(packages, data, (d, s) async {
      await subDirIter(s, d, _generateAction,
          filter: (d) => path.basename(d.path) == "mojom");
    });
  }

  /// Fetch mojoms for the package in |packageDirectory| if it has a .mojoms
  /// file.
  _fetchAction(SubDirIterData _, Directory packageDirectory) async {
    var packageUri = packageDirectory.uri;
    // TODO(zra): Also look in realpath(packageDirectory).parent for .mojoms
    // file.
    var mojomsPath = path.join(packageDirectory.path, '.mojoms');
    var mojomsFile = new File(mojomsPath);
    if (!await mojomsFile.exists()) return;
    if (_verbose) print("Found .mojoms file: $mojomsPath");
    await _fetchFromDotMojoms(packageUri, packageDirectory.parent, mojomsFile);
  }
}

/// Given the location of the Mojo SDK and a root directory from which to begin
/// a search. Find .mojoms files, and generate bindings for the containing
/// packages.
class TreeGenerator {
  MojomGenerator _generator;
  Directory _mojoSdk;
  Directory _rootDir;
  List<String> _skip;
  bool _verbose;
  bool _profile;
  bool _dryRun;
  int errors;

  TreeGenerator(Directory mojoSdk, Directory rootDir, List<String> skip,
      {bool verbose: false, bool profile: false, bool dryRun: false}) {
    _mojoSdk = mojoSdk;
    _rootDir = rootDir;
    _skip = skip;
    _verbose = verbose;
    _profile = profile;
    _dryRun = dryRun;
    _generator = new MojomGenerator(_mojoSdk,
        verbose: _verbose, profile: _profile, dryRun: _dryRun);
    errors = 0;
  }

  findAndGenerate() async {
    Set<String> alreadySeen = new Set<String>();
    await for (var entry in _rootDir.list(recursive: true)) {
      if (entry is! File) continue;
      if (!isDotMojoms(entry.path)) continue;
      if (alreadySeen.contains(entry.path)) continue;
      if (_shouldSkip(entry)) continue;
      alreadySeen.add(entry.path);
      await _runGenerate(entry.parent);
    }
    if (_profile) {
      _generator.printProfile();
    }
  }

  bool _shouldSkip(File f) {
    if (_skip == null) return false;
    var match =
        _skip.firstWhere((p) => f.path.startsWith(p), orElse: () => null);
    return match != null;
  }

  _runGenerate(Directory package) async {
    try {
      if (_verbose) print('Generating bindings for $package');
      await _generator.generateForPackage(package);
      if (_verbose) print('Done generating bindings for $package');
    } on GenerationError catch (e) {
      stderr.writeln('Bindings generation failed for package $package: $e');
      errors += 1;
    }
  }
}

/// Given the root of a directory tree to check, and the root of a directory
/// tree containing the canonical generated bindings, checks that the files
/// match, and recommends actions to take in case they don't. The canonical
/// directory should contain a subdirectory for each package that might be
/// encountered while traversing the directory tree being checked.
class TreeChecker {
  bool _verbose;
  bool _profile;
  Directory _mojoSdk;
  Directory _root;
  Directory _canonical;
  List<String> _skip;
  int _errors;

  TreeChecker(this._mojoSdk, this._root, this._canonical, this._skip,
      {bool verbose: false, bool profile: false})
      : _verbose = verbose,
        _profile = profile,
        _errors = 0;

  check() async {
    Set<String> alreadySeen = new Set<String>();
    await for (var entry in _root.list(recursive: true)) {
      if (entry is! File) continue;
      if (!isDotMojoms(entry.path)) continue;
      if (entry.path.startsWith(_canonical.path)) continue;

      String realpath = await entry.resolveSymbolicLinks();
      if (alreadySeen.contains(realpath)) continue;
      alreadySeen.add(realpath);

      if (_shouldSkip(entry)) continue;
      var parent = entry.parent;
      if (_verbose) print("Checking package at: ${parent.path}");

      await _checkAll(parent);
      await _checkSame(parent);
    }
    if (_errors > 1) {
      String dart = makeRelative(Platform.executable);
      String scriptPath = makeRelative(path.fromUri(Platform.script));
      String mojoSdk = makeRelative(_mojoSdk.path);
      String root = makeRelative(_root.path);
      stderr.writeln('It looks like there were multiple problems. '
          'You can run the following command to regenerate bindings for your '
          'whole tree:\n'
          '\t$dart $scriptPath tree -r $root -m $mojoSdk');
    }
  }

  int get errors => _errors;

  // Check that the files are the same.
  _checkSame(Directory package) async {
    Directory libDir = new Directory(path.join(package.path, 'lib'));
    Set<String> alreadySeen = new Set<String>();
    await for (var entry in libDir.list(recursive: true)) {
      if (entry is! File) continue;
      if (!isMojomDart(entry.path)) continue;
      String realpath = await entry.resolveSymbolicLinks();
      if (alreadySeen.contains(realpath)) continue;
      alreadySeen.add(realpath);

      String relPath = path.relative(entry.path, from: package.parent.path);
      File canonicalFile = new File(path.join(_canonical.path, relPath));
      if (!await canonicalFile.exists()) {
        if (_verbose) print("No canonical file for $entry");
        continue;
      }

      if (_verbose) print("Comparing $entry with $canonicalFile");
      int fileComparison = await compareFiles(entry, canonicalFile);
      if (fileComparison != 0) {
        String entryPath = makeRelative(entry.path);
        String canonicalPath = makeRelative(canonicalFile.path);
        if (fileComparison > 0) {
          stderr.writeln('The generated file:\n\t$entryPath\n'
              'is newer thanthe canonical file\n\t$canonicalPath\n,'
              'and they are different. Regenerate canonical files?');
        } else {
          String dart = makeRelative(Platform.executable);
          String packagePath = makeRelative(package.path);
          String scriptPath = makeRelative(path.fromUri(Platform.script));
          String mojoSdk = makeRelative(_mojoSdk.path);
          stderr.writeln('For the package: $packagePath\n'
              'The generated file:\n\t$entryPath\n'
              'is older than the canonical file:\n\t$canonicalPath\n'
              'and they are different. Regenerate by running:\n'
              '\t$dart $scriptPath single -p $packagePath -m $mojoSdk');
        }
        _errors++;
        return;
      }
    }
  }

  // Check that every .mojom.dart in the canonical package is also in the
  // package we are checking.
  _checkAll(Directory package) async {
    String packageName = path.relative(package.path, from: package.parent.path);
    String canonicalPackagePath =
        path.join(_canonical.path, packageName, 'lib');
    Directory canonicalPackage = new Directory(canonicalPackagePath);
    if (!await canonicalPackage.exists()) return;

    Set<String> alreadySeen = new Set<String>();
    await for (var entry in canonicalPackage.list(recursive: true)) {
      if (entry is! File) continue;
      if (!isMojomDart(entry.path)) continue;
      String realpath = await entry.resolveSymbolicLinks();
      if (alreadySeen.contains(realpath)) continue;
      alreadySeen.add(realpath);

      String relPath = path.relative(entry.path, from: canonicalPackage.path);
      File genFile = new File(path.join(package.path, 'lib', relPath));
      if (_verbose) print("Checking that $genFile exists");
      if (!await genFile.exists()) {
        String dart = makeRelative(Platform.executable);
        String genFilePath = makeRelative(genFile.path);
        String packagePath = makeRelative(package.path);
        String scriptPath = makeRelative(path.fromUri(Platform.script));
        String mojoSdk = makeRelative(_mojoSdk.path);
        stderr.writeln('The generated file:\n\t$genFilePath\n'
            'is needed but does not exist. Make sure that the .mojom to '
            'generate it is listed in the .mojoms file for package '
            '$packageName, and run the command\n'
            '\t$dart $scriptPath single -p $packagePath -m $mojoSdk');
        _errors++;
      }
    }
  }

  bool _shouldSkip(File f) {
    if (_skip == null) return false;
    var match =
        _skip.firstWhere((p) => f.path.startsWith(p), orElse: () => null);
    return match != null;
  }
}
