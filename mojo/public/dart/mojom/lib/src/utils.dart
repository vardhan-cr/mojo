// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library mojom.utils;

import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:crypto/crypto.dart' as crypto;
import 'package:path/path.dart' as path;

bool isMojomDart(String path) => path.endsWith('.mojom.dart');
bool isMojom(String path) => path.endsWith('.mojom');
bool isDotMojoms(String path) => path.endsWith(".mojoms");

String makeAbsolute(String p) =>
    path.isAbsolute(p) ? path.normalize(p) : path.normalize(path.absolute(p));

String makeRelative(String p) => path.isAbsolute(p)
    ? path.normalize(path.relative(p, from: Directory.current.path))
    : path.normalize(p);

/// An Error for problems on the command line.
class CommandLineError {
  final _msg;
  CommandLineError(this._msg);
  toString() => "Command Line Error: $_msg";
}

/// An Error for failures of the bindings generation script.
class GenerationError {
  final _msg;
  GenerationError(this._msg);
  toString() => "Generation Error: $_msg";
}

/// An Error for failing to fetch a .mojom file.
class FetchError {
  final _msg;
  FetchError(this._msg);
  toString() => "Fetch Error: $_msg";
}

class SubDirIterData {
  Directory subdir;
  SubDirIterData(this.subdir);
}

typedef Future SubDirAction(SubDirIterData data, Directory subdir);

typedef bool DirectoryFilter(Directory d);

/// Iterates over subdirectories of |directory| that satisfy |filter| if one
/// is given. Applies |action| to each subdirectory passing it |data| and the
/// subdirectory. Recurses into further subdirectories if |recursive| is true.
subDirIter(Directory directory, SubDirIterData data, SubDirAction action,
    {DirectoryFilter filter, bool recursive: false}) async {
  await for (var subdir in directory.list(recursive: recursive)) {
    if (subdir is! Directory) continue;
    if ((filter != null) && !filter(subdir)) continue;
    if (data != null) {
      data.subdir = subdir;
    }
    await action(data, subdir);
  }
}

Future<String> _downloadUrl(HttpClient httpClient, Uri uri) async {
  try {
    var request = await httpClient.getUrl(uri);
    var response = await request.close();
    if (response.statusCode >= 400) {
      var msg = "Failed to download $uri\nCode ${response.statusCode}";
      if (response.reasonPhrase != null) {
        msg = "$msg: ${response.reasonPhrase}";
      }
      throw new FetchError(msg);
    }
    var fileString = new StringBuffer();
    await for (String contents in response.transform(UTF8.decoder)) {
      fileString.write(contents);
    }
    return fileString.toString();
  } catch (e) {
    throw new FetchError("$e");
  }
}

/// Fetch file at [uri] using [httpClient] if needed. Throw a [FetchError] if
/// the file is not successfully fetched.
Future<String> fetchUri(HttpClient httpClient, Uri uri) async {
  if (uri.scheme.startsWith('http')) {
    return _downloadUrl(httpClient, uri);
  }
  try {
    File f = new File.fromUri(uri);
    return await f.readAsString();
  } catch (e) {
    throw new FetchError("$e");
  }
}

markFileReadOnly(String file) async {
  if (!Platform.isLinux && !Platform.isMacOS) {
    String os = Platform.operatingSystem;
    throw 'Setting file $file read-only is not supported on $os.';
  }
  var process = await Process.run('chmod', ['a-w', file]);
  if (process.exitCode != 0) {
    print('Setting file $file read-only failed: ${process.stderr}');
  }
}

Future<DateTime> getModificationTime(Uri uri) async {
  if (uri.scheme.startsWith('http')) {
    return new DateTime.now();
  }
  File f = new File.fromUri(uri);
  var stat = await f.stat();
  return stat.modified;
}

Future<List<int>> fileSHA1(File f) async {
  var sha1 = new crypto.SHA1();
  await for (var bytes in f.openRead()) {
    sha1.add(bytes);
  }
  return sha1.close();
}

Future<bool> equalSHA1(File file1, File file2) async {
  List<int> file1sha1 = await fileSHA1(file1);
  List<int> file2sha1 = await fileSHA1(file2);
  if (file1sha1.length != file2sha1.length) return false;
  for (int i = 0; i < file1sha1.length; i++) {
    if (file1sha1[i] != file2sha1[i]) return false;
  }
  return true;
}

/// If the files are the same, returns 0.
/// Otherwise, returns a negative number if f1 less recently modified than f2,
/// or a positive number if f1 is more recently modified than f2.
Future<int> compareFiles(File f1, File f2) async {
  FileStat f1stat = await f1.stat();
  FileStat f2stat = await f2.stat();
  if ((f1stat.size != f2stat.size) || !await equalSHA1(f1, f2)) {
    return (f1stat.modified.isBefore(f2stat.modified)) ? -1 : 1;
  }
  return 0;
}
