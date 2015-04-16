// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library dart_embedder_packages;

// We import these here so they are slurped up into the snapshot. Only packages
// copied by the "dart_embedder_packages" rule can be imported here.

// The embedder-package: is a non-standard scheme that only works because of
// --url_mapping arguments passed to gen_snapshot. If you need to add new
// embedder packages, you must also change the snapshot generation rule.
// See HACKING.md for more information.

import 'embedder-package:mojo/public/dart/application.dart';
import 'embedder-package:mojo/public/dart/bindings.dart';
import 'embedder-package:mojo/public/dart/core.dart';
import 'embedder-package:mojo/net_address.mojom.dart';
import 'embedder-package:mojo/network_error.mojom.dart';
import 'embedder-package:mojo/network_service.mojom.dart';
import 'embedder-package:mojo/tcp_bound_socket.mojom.dart';
import 'embedder-package:mojo/tcp_connected_socket.mojom.dart';
import 'embedder-package:mojo/tcp_server_socket.mojom.dart';
