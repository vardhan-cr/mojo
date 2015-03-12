#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import logging
import os
import shutil
import subprocess
import sys

SKY_TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
SKY_DIR = os.path.dirname(SKY_TOOLS_DIR)
SRC_ROOT = os.path.dirname(SKY_DIR)


def git_revision():
    return subprocess.check_output(['git', 'rev-parse', 'HEAD']).strip()


def mojo_filter(path):
    if not os.path.isfile(path):
        return False
    _, ext = os.path.splitext(path)
    if ext != '.mojo':
        return False
    return 'apptests' not in os.path.basename(path)


def gen_filter(path):
    if os.path.isdir(path):
        return True
    _, ext = os.path.splitext(path)
    # Don't include all .dart, just .mojom.dart.
    return path.endswith('.mojom.dart')

def dart_filter(path):
    if os.path.isdir(path):
        return True
    _, ext = os.path.splitext(path)
    # .dart includes '.mojom.dart'
    return ext == '.dart'


def sky_or_dart_filter(path):
    if os.path.isdir(path):
        return True
    _, ext = os.path.splitext(path)
    # .dart includes '.mojom.dart'
    return ext == '.sky' or ext == '.dart'


def ensure_dir_exists(path):
    if not os.path.exists(path):
        os.makedirs(path)


def copy(from_root, to_root, filter_func=None):
    ensure_dir_exists(to_root)

    for root, dirs, files in os.walk(from_root):
        # filter_func expects paths not names, so wrap it to make them absolute.
        wrapped_filter = None
        if filter_func:
            wrapped_filter = lambda name: filter_func(os.path.join(root, name))

        for name in filter(wrapped_filter, files):
            from_path = os.path.join(root, name)
            root_rel_path = os.path.relpath(from_path, from_root)
            to_path = os.path.join(to_root, root_rel_path)
            to_dir = os.path.dirname(to_path)
            if not os.path.exists(to_dir):
                os.makedirs(to_dir)
            shutil.copy(from_path, to_path)

        dirs[:] = filter(wrapped_filter, dirs)


def confirm(prompt):
    response = raw_input('%s [N]|y: ' % prompt)
    return response and response.lower() == 'y'


def delete_all_non_hidden_files_in_directory(root):
    to_delete = [os.path.join(root, p)
        for p in os.listdir(root) if not p.startswith('.')]
    if not confirm('This will delete everything in %s:\n%s\nAre you sure?' % (
        root, '\n'.join(to_delete))):
        print 'User aborted.'
        sys.exit(2)

    for path in to_delete:
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)


def main():
    logging.basicConfig(level=logging.WARN)
    parser = argparse.ArgumentParser(description='Deploy a new sky_sdk.')
    parser.add_argument('sdk_root', type=str)
    args = parser.parse_args()

    # Always use android release?
    rel_build_dir = os.path.join('out', 'android_Release')
    build_dir = os.path.join(SRC_ROOT, rel_build_dir)
    sdk_root = os.path.abspath(args.sdk_root)

    def sdk_path(rel_path):
        return os.path.join(sdk_root, rel_path)

    def src_path(rel_path):
        return os.path.join(SRC_ROOT, rel_path)

    delete_all_non_hidden_files_in_directory(sdk_root)

    # Manually clear sdk_root above to avoid deleting dot-files.
    copy(src_path('sky/sdk'), sdk_root)

    copy(src_path('sky/examples'), sdk_path('examples'),
        sky_or_dart_filter)

    # Sky package
    copy(src_path('sky/framework'), sdk_path('packages/sky/lib/framework'),
        sky_or_dart_filter)
    copy(src_path('sky/assets'), sdk_path('packages/sky/lib/assets'))
    copy(src_path('sky/sdk/tools/sky'), sdk_path('packages/sky/bin/sky'))
    copy(os.path.join(build_dir, 'gen/sky'), sdk_path('packages/sky/lib'),
        gen_filter)

    # Mojo package
    copy(src_path('mojo/public'), sdk_path('packages/mojo/lib/public'), dart_filter)
    copy(os.path.join(build_dir, 'gen/mojo'), sdk_path('packages/mojo/lib'),
        gen_filter)


    ensure_dir_exists(sdk_path('apks'))
    shutil.copy(os.path.join(build_dir, 'apks', 'SkyShell.apk'),
        sdk_path('apks'))


    with open(sdk_path('LICENSES.sky'), 'w') as license_file:
        subprocess.check_call([src_path('tools/licenses.py'), 'credits'],
            stdout=license_file)


    subprocess.check_call(['git', 'add', '.'], cwd=sdk_root)
    subprocess.check_call([
        'git', 'commit',
        '-m', '%s from %s' % (rel_build_dir, git_revision())
        ], cwd=sdk_root)


if __name__ == '__main__':
    main()
