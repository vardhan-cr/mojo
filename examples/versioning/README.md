Mojom Versioning Example
========================

This example demonstrates how to evolve Mojom definitions in a backward
compatible way, so that apps using different versions of those Mojom definitions
can still communicate with each other. This example use a fictitious human
resource management system as example.

// TODO(yzshen): Demonstrate interface versioning when the feature is ready.

- hr_system_{client, server}.mojom: Mojom definitions used by the client side
  and the server side, respectively. Some of those definitions are at different
  versions.
- hr_system_client.cc: client of the HR system, built against
  hr_system_client.mojom.
- hr_system_server.cc: server of the HR system, built against
  hr_system_server.mojom.
