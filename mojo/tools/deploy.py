#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import logging
import os
import shutil
import subprocess

from mopy.paths import Paths


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
    return ext == '.sky' or path.endswith('.mojom.dart')

def sky_or_dart_filter(path):
    if os.path.isdir(path):
        return True
    _, ext = os.path.splitext(path)
    # .dart includes '.mojom.dart'
    return ext == '.sky' or ext == '.dart'


def copy(from_root, to_root, filter_func=None):
    if os.path.exists(to_root):
        shutil.rmtree(to_root)
    os.makedirs(to_root)

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
            shutil.copyfile(from_path, to_path)

        dirs[:] = filter(wrapped_filter, dirs)

def main():
    logging.basicConfig(level=logging.WARN)
    parser = argparse.ArgumentParser(description='Deploy a new build of mojo.')
    parser.add_argument('deploy_root', type=str)
    args = parser.parse_args()

    # Always use android release?
    rel_build_dir = os.path.join('out', 'android_Release')
    build_dir = os.path.join(Paths().src_root, rel_build_dir)
    paths = Paths(build_dir=build_dir)

    def deploy_path(rel_path):
        return os.path.join(args.deploy_root, rel_path)

    def src_path(rel_path):
        return os.path.join(paths.src_root, rel_path)

    copy(paths.build_dir, deploy_path('mojo'), mojo_filter)
    copy(src_path('sky/examples'), deploy_path('sky/examples'),
        sky_or_dart_filter)
    copy(src_path('sky/framework'), deploy_path('sky/framework'),
        sky_or_dart_filter)
    copy(os.path.join(paths.build_dir, 'gen'), deploy_path('gen'), gen_filter)

    shutil.copy(os.path.join(paths.build_dir, 'apks', 'MojoShell.apk'),
        args.deploy_root)
    shutil.copy(os.path.join(paths.build_dir, 'apks', 'MojoShortcuts.apk'),
        args.deploy_root)
    subprocess.check_call(['git', 'add', '.'], cwd=args.deploy_root)
    subprocess.check_call([
        'git', 'commit',
        '-m', '%s from %s' % (rel_build_dir, git_revision())
        ], cwd=args.deploy_root)



if __name__ == '__main__':
    main()
