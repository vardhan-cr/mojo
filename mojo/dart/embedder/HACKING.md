Dart Embedder Hacking
====

## Debugging

Under Mojo, by default the Dart VM is built in Release mode regardless of the
mode that Mojo is built in. That is, when Mojo is built in Debug mode, the
Dart VM is still built in Release mode. To change this behavior while working
on the embedder, pass dart_debug=true to gn to configure a Debug build of the
Dart VM. I.e.:

```
$ gn gen --check out/Debug --args='... dart_debug=true'
```
