// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:io';

import 'package:mojom/src/command_runner.dart';
import 'package:mojom/src/utils.dart';
import 'package:path/path.dart' as path;
import 'package:unittest/unittest.dart';

final mojomContents = '''
[DartPackage="generated"]
module generated;

struct Transform {
  // Row major order.
  array<float, 16> matrix;
};
''';

final dldMojomContents1 = '''
[DartPackage="downloaded"]
module downloaded;

struct Downloaded1 {
  int32 status;
};
''';

final dldMojomContents2 = '''
[DartPackage="downloaded"]
module downloaded;

struct Downloaded2 {
  int32 status;
};
''';

final singlePacakgeMojomContents = '''
[DartPackage="single_package"]
module single_package;
struct SinglePackage {
  int32 thingo;
};
''';

Future runCommand(List<String> args) {
  return new MojomCommandRunner().run(args);
}

main() async {
  String mojoSdk;
  if (Platform.environment['MOJO_SDK'] != null) {
    mojoSdk = Platform.environment['MOJO_SDK'];
  } else {
    mojoSdk = path.normalize(
        path.join(path.dirname(Platform.script.path), '..', '..', '..'));
  }
  if (!await new Directory(mojoSdk).exists()) {
    fail("Could not find the Mojo SDK");
  }

  final scriptPath = path.dirname(Platform.script.path);

  // //test_packages/mojom
  final testPackagePath = path.join(scriptPath, 'test_packages');
  final testMojomPath = path.join(testPackagePath, 'mojom');
  // //test_packages/pregen/mojom/pregen/pregen.mojom.dart
  final pregenPath = path.join(testPackagePath, 'pregen');
  final pregenFilePath =
      path.join(pregenPath, 'mojom', 'pregen', 'pregen.mojom.dart');
  // //test_packages/generated
  final generatedPackagePath = path.join(testPackagePath, 'generated');
  // //test_packages/downloaded/.mojoms
  final downloadedPackagePath = path.join(testPackagePath, 'downloaded');
  final dotMojomsPath = path.join(downloadedPackagePath, '.mojoms');

  // //mojom_link_target/lib/dart
  final testMojomLinkPath = path.join(scriptPath, 'mojom_link_target');
  final testMojomLibPath = path.join(testMojomLinkPath, 'lib');
  final fakeGeneratePath = path.join(testMojomLibPath, 'dart');

  // //additional_dir/additional/additional.mojom.dart
  final additionalRootPath = path.join(scriptPath, 'additional_dir');
  final additionalPath =
      path.join(additionalRootPath, 'additional', 'additional.mojom.dart');

  // //single_package
  final singlePackagePath = path.join(scriptPath, 'single_package');
  // //single_package/.mojoms
  final singlePackageMojomsPath = path.join(singlePackagePath, '.mojoms');
  // //single_package/lib
  final singlePackageLibPath = path.join(singlePackagePath, 'lib');
  // //single_package/packages
  final singlePackagePackagesPath = path.join(singlePackagePath, 'packages');
  // //single_package/packages/single_package
  final singlePackagePackagePath =
      path.join(singlePackagePackagesPath, 'single_package');
  final singlePackageMojomPackagePath =
      path.join(singlePackagePackagesPath, 'mojom');

  setUp(() async {
    await new File(pregenFilePath).create(recursive: true);
    await new File(additionalPath).create(recursive: true);
    await new File(fakeGeneratePath).create(recursive: true);
    await new Link(testMojomPath).create(testMojomLibPath);

    // //test_packages/generated/mojom/generated/public/interfaces
    //     /generated.mojom
    final generatedMojomFile = new File(path.join(testPackagePath, 'generated',
        'mojom', 'generated', 'public', 'interfaces', 'generated.mojom'));
    await generatedMojomFile.create(recursive: true);
    await generatedMojomFile.writeAsString(mojomContents);

    await new Directory(downloadedPackagePath).create(recursive: true);
  });

  tearDown(() async {
    await new Directory(additionalRootPath).delete(recursive: true);
    await new Directory(testPackagePath).delete(recursive: true);
    await new Directory(testMojomLinkPath).delete(recursive: true);
  });

  group('No Fetch', () {
    test('No-op', () async {
      await runCommand(['all', '-p', testPackagePath, '-m', mojoSdk]);
      final mojomPackageDir = new Directory(testMojomPath);
      final generateFile = new File(path.join(testMojomPath, 'dart'));
      expect(await mojomPackageDir.exists(), isTrue);
      expect(await generateFile.exists(), isTrue);
    });

    test('Additional', () async {
      await runCommand([
        'all',
        '-p',
        testPackagePath,
        '-m',
        mojoSdk,
        '-a',
        additionalRootPath
      ]);
      final mojomPackageDir = new Directory(testMojomPath);
      final generateFile = new File(path.join(testMojomPath, 'dart'));
      final additionalFile = new File(
          path.join(testMojomPath, 'additional', 'additional.mojom.dart'));
      expect(await mojomPackageDir.exists(), isTrue);
      expect(await generateFile.exists(), isTrue);
      expect(await additionalFile.exists(), isTrue);
    });

    test('Generated', () async {
      await runCommand(['all', '-g', '-p', testPackagePath, '-m', mojoSdk]);
      final generatedFile = new File(
          path.join(generatedPackagePath, 'generated', 'generated.mojom.dart'));
      expect(await generatedFile.exists(), isTrue);
    });

    test('All', () async {
      await runCommand([
        'all',
        '-g',
        '-p',
        testPackagePath,
        '-m',
        mojoSdk,
        '-a',
        additionalRootPath
      ]);

      final additionalFile = new File(
          path.join(testMojomPath, 'additional', 'additional.mojom.dart'));
      expect(await additionalFile.exists(), isTrue);

      final generatedFile = new File(
          path.join(generatedPackagePath, 'generated', 'generated.mojom.dart'));
      expect(await generatedFile.exists(), isTrue);
    });
  });

  group('Fetch', () {
    var httpServer;
    setUp(() async {
      httpServer = await HttpServer.bind("localhost", 0);
      httpServer.listen((HttpRequest request) {
        String path = request.uri.path;
        if (path.endsWith('path/to/mojom/download_one.mojom')) {
          request.response.write(dldMojomContents1);
        } else if (path.endsWith('path/to/mojom/download_two.mojom')) {
          request.response.write(dldMojomContents2);
        } else {
          request.response.statusCode = HttpStatus.NOT_FOUND;
        }
        request.response.close();
      });
    });

    tearDown(() async {
      await httpServer.close();
      httpServer = null;
    });

    test('simple', () async {
      final mojomsFile = new File(dotMojomsPath);
      await mojomsFile.create(recursive: true);
      await mojomsFile
          .writeAsString("root: http://localhost:${httpServer.port}\n"
              "path/to/mojom/download_one.mojom\n");
      await runCommand(
          ['all', '-p', testPackagePath, '-m', mojoSdk, '-f', '-g']);
      final downloadedFile = new File(path.join(
          downloadedPackagePath, 'downloaded', 'download_one.mojom.dart'));
      expect(await downloadedFile.exists(), isTrue);
      await mojomsFile.delete();
    });

    test('two files', () async {
      final mojomsFile = new File(dotMojomsPath);
      await mojomsFile.create(recursive: true);
      await mojomsFile
          .writeAsString("root: http://localhost:${httpServer.port}\n"
              "path/to/mojom/download_one.mojom\n"
              "path/to/mojom/download_two.mojom\n");
      await runCommand(
          ['all', '-p', testPackagePath, '-m', mojoSdk, '-f', '-g']);
      final downloaded1File = new File(path.join(
          downloadedPackagePath, 'downloaded', 'download_one.mojom.dart'));
      expect(await downloaded1File.exists(), isTrue);
      final downloaded2File = new File(path.join(
          downloadedPackagePath, 'downloaded', 'download_two.mojom.dart'));
      expect(await downloaded2File.exists(), isTrue);
      await mojomsFile.delete();
    });

    test('two roots', () async {
      final mojomsFile = new File(dotMojomsPath);
      await mojomsFile.create(recursive: true);
      await mojomsFile
          .writeAsString("root: http://localhost:${httpServer.port}\n"
              "path/to/mojom/download_one.mojom\n"
              "root: http://localhost:${httpServer.port}\n"
              "path/to/mojom/download_two.mojom\n");
      await runCommand(
          ['all', '-p', testPackagePath, '-m', mojoSdk, '-f', '-g']);
      final downloaded1File = new File(path.join(
          downloadedPackagePath, 'downloaded', 'download_one.mojom.dart'));
      expect(await downloaded1File.exists(), isTrue);
      final downloaded2File = new File(path.join(
          downloadedPackagePath, 'downloaded', 'download_two.mojom.dart'));
      expect(await downloaded2File.exists(), isTrue);
      await mojomsFile.delete();
    });

    test('simple-comment', () async {
      final mojomsFile = new File(dotMojomsPath);
      await mojomsFile.create(recursive: true);
      await mojomsFile.writeAsString("# Comments are allowed\n"
          " root: http://localhost:${httpServer.port}\n\n\n\n"
          "          # Here too\n"
          " path/to/mojom/download_one.mojom\n"
          "# And here\n");
      await runCommand(
          ['all', '-p', testPackagePath, '-m', mojoSdk, '-f', '-g']);
      final downloadedFile = new File(path.join(
          downloadedPackagePath, 'downloaded', 'download_one.mojom.dart'));
      expect(await downloadedFile.exists(), isTrue);
      await mojomsFile.delete();
    });

    test('404', () async {
      final mojomsFile = new File(dotMojomsPath);
      await mojomsFile.create(recursive: true);
      await mojomsFile
          .writeAsString("root: http://localhost:${httpServer.port}\n"
              "blah\n");
      var fail = false;
      try {
        await runCommand(
            ['all', '-p', testPackagePath, '-m', mojoSdk, '-f', '-g']);
      } on FetchError {
        fail = true;
      }
      expect(fail, isTrue);
      await mojomsFile.delete();
    });
  });

  group('Single Pacakge', () {
    setUp(() async {
      await new Directory(singlePackageLibPath).create(recursive: true);
      await new Directory(singlePackagePackagesPath).create(recursive: true);
      await new Directory(singlePackageMojomPackagePath)
          .create(recursive: true);
      await new Link(singlePackagePackagePath).create(singlePackageLibPath);

      // //test_packages/single_package/public/interfaces/single_package.mojom
      final singlePackageMojomFile = new File(path.join(testPackagePath,
          'single_package', 'public', 'interfaces', 'single_package.mojom'));
      await singlePackageMojomFile.create(recursive: true);
      await singlePackageMojomFile.writeAsString(singlePacakgeMojomContents);
    });

    tearDown(() async {
      await new Directory(singlePackagePath).delete(recursive: true);
    });

    test('relative path', () async {
      final mojomsFile = new File(singlePackageMojomsPath);
      await mojomsFile.create(recursive: true);
      await mojomsFile.writeAsString("root: ../test_packages/\n"
          "single_package/public/interfaces/single_package.mojom\n");
      await runCommand(['single', '-m', mojoSdk, '-p', singlePackagePath]);

      // Should have:
      // //single_package/lib/single_package/single_package.mojom.dart
      final resultPath = path.join(
          singlePackageLibPath, 'single_package', 'single_package.mojom.dart');
      final resultFile = new File(resultPath);
      expect(await resultFile.exists(), isTrue);
    });

    test('absolute path', () async {
      final mojomsFile = new File(singlePackageMojomsPath);
      await mojomsFile.create(recursive: true);
      await mojomsFile.writeAsString("root: $testPackagePath/\n"
          "single_package/public/interfaces/single_package.mojom\n");
      await runCommand(['single', '-m', mojoSdk, '-p', singlePackagePath]);

      // Should have:
      // //single_package/lib/single_package/single_package.mojom.dart
      final resultPath = path.join(
          singlePackageLibPath, 'single_package', 'single_package.mojom.dart');
      final resultFile = new File(resultPath);
      expect(await resultFile.exists(), isTrue);
    });

    test('file:', () async {
      final mojomsFile = new File(singlePackageMojomsPath);
      await mojomsFile.create(recursive: true);
      await mojomsFile.writeAsString("root: file://$testPackagePath/\n"
          "single_package/public/interfaces/single_package.mojom\n");
      await runCommand(['single', '-m', mojoSdk, '-p', singlePackagePath]);

      // Should have:
      // //single_package/lib/single_package/single_package.mojom.dart
      final resultPath = path.join(
          singlePackageLibPath, 'single_package', 'single_package.mojom.dart');
      final resultFile = new File(resultPath);
      expect(await resultFile.exists(), isTrue);
    });

    test('Not found', () async {
      final mojomsFile = new File(singlePackageMojomsPath);
      await mojomsFile.create(recursive: true);
      await mojomsFile.writeAsString("root: file://$testPackagePath/\n"
          "this/does/not/exist/single_package.mojom\n");
      var fail = false;
      try {
        await runCommand(['single', '-m', mojoSdk, '-p', singlePackagePath]);
      } on FetchError {
        fail = true;
      }
      expect(fail, isTrue);
    });
  });

  group('Failures', () {
    test('Bad Package Root', () async {
      final dummyPackageRoot = path.join(scriptPath, 'dummyPackageRoot');
      var fail = false;
      try {
        await runCommand(['all', '-p', dummyPackageRoot, '-m', mojoSdk]);
      } on CommandLineError {
        fail = true;
      }
      expect(fail, isTrue);
    });

    test('Non-absolute PackageRoot', () async {
      final dummyPackageRoot = 'dummyPackageRoot';
      var fail = false;
      try {
        await runCommand(['all', '-p', dummyPackageRoot, '-m', mojoSdk]);
      } on CommandLineError {
        fail = true;
      }
      expect(fail, isTrue);
    });

    test('Bad Additional Dir', () async {
      final dummyAdditional = path.join(scriptPath, 'dummyAdditional');
      var fail = false;
      try {
        await runCommand([
          'all',
          '-a',
          dummyAdditional,
          '-p',
          testPackagePath,
          '-m',
          mojoSdk
        ]);
      } on CommandLineError {
        fail = true;
      }
      expect(fail, isTrue);
    });

    test('Non-absolute Additional Dir', () async {
      final dummyAdditional = 'dummyAdditional';
      var fail = false;
      try {
        await runCommand([
          'all',
          '-a',
          dummyAdditional,
          '-p',
          testPackagePath,
          '-m',
          mojoSdk
        ]);
      } on CommandLineError {
        fail = true;
      }
      expect(fail, isTrue);
    });

    test('No Mojo Package', () async {
      final dummyPackageRoot = path.join(scriptPath, 'dummyPackageRoot');
      final dummyPackageDir = new Directory(dummyPackageRoot);
      await dummyPackageDir.create(recursive: true);

      var fail = false;
      try {
        await runCommand(['all', '-p', dummyPackageRoot, '-m', mojoSdk]);
      } on CommandLineError {
        fail = true;
      }
      await dummyPackageDir.delete(recursive: true);
      expect(fail, isTrue);
    });

    test('Bad Mojo SDK', () async {
      final dummySdk = path.join(scriptPath, 'dummySdk');
      var fail = false;
      try {
        await runCommand(['all', '-g', '-m', dummySdk, '-p', testPackagePath]);
      } on CommandLineError {
        fail = true;
      }
      expect(fail, isTrue);
    });

    test('Download No Server', () async {
      final mojomsFile = new File(dotMojomsPath);
      await mojomsFile.create(recursive: true);
      await mojomsFile.writeAsString("root: http://localhots\n"
          "path/to/mojom/download_one.mojom\n");
      var fail = false;
      try {
        await runCommand(
            ['all', '-p', testPackagePath, '-m', mojoSdk, '-f', '-g']);
      } on FetchError {
        fail = true;
      }
      expect(fail, isTrue);
      await mojomsFile.delete();
    });

    test('.mojoms no root', () async {
      final mojomsFile = new File(dotMojomsPath);
      await mojomsFile.create(recursive: true);
      await mojomsFile.writeAsString("path/to/mojom/download_one.mojom\n");
      var fail = false;
      try {
        await runCommand(
            ['all', '-p', testPackagePath, '-m', mojoSdk, '-f', '-g']);
      } on FetchError {
        fail = true;
      }
      expect(fail, isTrue);
      await mojomsFile.delete();
    });

    test('.mojoms blank root', () async {
      final mojomsFile = new File(dotMojomsPath);
      await mojomsFile.create(recursive: true);
      await mojomsFile.writeAsString("root:\n"
          "path/to/mojom/download_one.mojom\n");
      var fail = false;
      try {
        await runCommand(
            ['all', '-p', testPackagePath, '-m', mojoSdk, '-f', '-g']);
      } on FetchError {
        fail = true;
      }
      expect(fail, isTrue);
      await mojomsFile.delete();
    });

    test('.mojoms root without mojom', () async {
      final mojomsFile = new File(dotMojomsPath);
      await mojomsFile.create(recursive: true);
      await mojomsFile.writeAsString("root: http://localhost\n"
          "root: http://localhost\n"
          "path/to/mojom/download_one.mojom\n");
      var fail = false;
      try {
        await runCommand(
            ['all', '-p', testPackagePath, '-m', mojoSdk, '-f', '-g']);
      } on FetchError {
        fail = true;
      }
      expect(fail, isTrue);
      await mojomsFile.delete();
    });
  });
}
