// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

part of generate;

bool isMojomDart(String path) => path.endsWith('.mojom.dart');
bool isMojom(String path) => path.endsWith('.mojom');
bool isDotMojoms(String path) => path.endsWith(".mojoms");

/// An Error for problems on the command line.
class CommandLineError extends Error {
  final _msg;
  CommandLineError(this._msg);
  toString() => _msg;
}

/// An Error for failures of the bindings generation script.
class GenerationError extends Error {
  final _msg;
  GenerationError(this._msg);
  toString() => _msg;
}

/// An Error for failing to fetch a .mojom file.
class FetchError extends Error {
  final _msg;
  FetchError(this._msg);
  toString() => _msg;
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

getModificationTime(Uri uri) async {
  if (uri.scheme.startsWith('http')) {
    return new DateTime.now();
  }
  File f = new File.fromUri(uri);
  var stat = await f.stat();
  return stat.modified;
}
