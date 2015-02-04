#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# TODO(blundell): Eliminate this file once the bots are invoking
# upload_binaries.py directly.

import sys
import upload_binaries

if __name__ == "__main__":
  sys.exit(upload_binaries.main())
