# Devtools

Unopinionated tools for running, debugging and testing Mojo apps available to
Mojo consumers.

## Principles

The toolset is intended for consumption by Mojo consumers as a separate
checkout. No dependencies on files outside of devtools are allowed.

The toolset makes no assumptions about particular build system or file layout
in any checkout where it is consumed. Consumers can add thin wrappers adding a
layer of convenience / opinion on top of it.
