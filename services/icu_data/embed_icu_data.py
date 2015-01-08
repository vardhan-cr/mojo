#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
import hashlib

in_file = sys.argv[1]
out_file = sys.argv[2]

out_dir = os.path.dirname(out_file)

data = None
with open(in_file, "rb") as f:
    data = f.read()

if not os.path.exists(out_dir):
    os.makedirs(out_dir)

sha1hash = hashlib.sha1(data).hexdigest()

values = ["0x%02x" % ord(c) for c in data]
lines = []
chunk_size = 16
for i in range(0, len(values), chunk_size):
    lines.append(", ".join(values[i: i + chunk_size]))

with open(out_file, "w") as f:
    f.write('#include "services/icu_data/data.h"\n')
    f.write("namespace icu_data {\n")
    f.write("const char kICUDataTable[] = {\n")
    f.write(",\n".join(lines))
    f.write("\n};\n")
    f.write("const size_t kICUDataTableSize = sizeof(kICUDataTable);\n")
    f.write("const char kICUDataTableHash[] = \"%s\";\n" % sha1hash)
    f.write("}\n")
