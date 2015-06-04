# Devtools

Unopinionated tools for **running**, **debugging** and **testing** Mojo apps.

The repo consists of:

 - **devtoolslib** - Python module containing the core scripting functionality
   for running Mojo apps: shell abstraction with implementations for Android and
   Linux and support for apptest frameworks
 - executable scripts - example embedders of devtoolslib and other utils

## Embedding devtoolslib

As devtools carry no assumptions about build system or file layout being used,
the primary way of using devtools now is to embed the functionality provided by
**devtoolslib** in a thin wrapper script. For examples, one can refer to mojo's
[shell
runner](https://github.com/domokit/mojo/blob/master/mojo/tools/mojo_shell.py)
and [apptest
runner](https://github.com/domokit/mojo/blob/master/mojo/tools/apptest_runner.py).

## Executable scripts

The set of executable scripts is WIP. We currently offer:

 - **debugger** - allowing to send commands to the mojo:debugger app running in
   the shell, allowing e.g. to interactively start and stop tracing

## Install

```
git clone https://github.com/domokit/devtools.git
```

## File location

The library is canonically developed [in the mojo
repository](https://github.com/domokit/mojo/tree/master/mojo/devtools/common),
https://github.com/domokit/devtools is a mirror allowing to consume it
separately.
