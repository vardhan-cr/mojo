Mojo
====

Mojo is an effort to extract a common platform out of Chrome's renderer and
plugin processes that can support multiple types of sandboxed content, such as
HTML, Pepper, or NaCl.

## Set-up and code check-out

The instructions below only need to be done once. Note that a simple "git clone"
command is not sufficient to build the source code because this repo uses the
gclient command from depot_tools to manage most third party dependencies.

1. [Download
   depot_tools](http://www.chromium.org/developers/how-tos/install-depot-tools)
   and make sure it is in your path.
2. [Googlers only] Install Goma in ~/goma.
3. Create a directory somewhere for your checkout (preferably on an SSD), cd
   into it, and run the following commands:


```
$ fetch mojo # append --target_os=android to include Android build support.
$ cd src

# Or install-build-deps-android.sh if you plan to build for Android.
$ ./build/install-build-deps.sh

$ mojo/tools/mojob.py gn
```

The `fetch mojo` command does the following:
- creates a directory called 'src' under your checkout directory
- clones the repository using git clone
- clones dependencies with gclient sync

`install-build-deps.sh` installs any packages needed to build, then
`mojo/tools/mojob.py gn` runs `gn args` and configures the build directory,
out/Debug.

If the fetch command fails, you will need to delete the src directory and start
over.

### <a name="configure-android"></a>Adding Android bits in an existing checkout

If you configured your set-up for Linux and now wish to build for Android, edit
the `.gclient` file in your root Mojo directory (the parent directory to src.)
and add this line at the end of the file:

```
target_os = [u'android',u'linux']
```

Bring in Android-specific build dependencies:
```
$ build/install-build-deps-android.sh 
```

Pull down all of the packages with this command:
```
$ gclient sync
```

## <a name="buildmojo"></a>Build Mojo

### Linux

Build Mojo for Linux by running:

```
$ ninja -C out/Debug -j 10
```

You can also use the `mojob.py` script for building. This script automatically
calls ninja and sets -j to an appropriate value based on whether Goma (see the
section on Goma below) is present. You cannot specify a target name with this
script.

```
mojo/tools/mojob.py gn
mojo/tools/mojob.py build
```

Run a demo:
```
out/Debug/mojo_shell mojo:spinning_cube
```

Run the tests:
```
mojo/tools/mojob.py test
```

Create a release build:
```
mojo/tools/mojob.py gn --release
mojo/tools/mojob.py build --release
mojo/tools/mojob.py test --release
```

### Android

To build for Android, first make sure that your checkout is [configured](#configure-android) to build
for Android. After that you can use the mojob script as follows:
```
$ mojo/tools/mojob.py gn --android
$ mojo/tools/mojob.py build --android
```

The result will be in out/android_Debug. If you see javac compile errors,
[make sure you have an up-to-date JDK](https://code.google.com/p/chromium/wiki/AndroidBuildInstructions#Install_Java_JDK)

### Goma (Googlers only)

If you're a Googler, you can use Goma, a distributed compiler service for
open-source projects such as Chrome and Android. If Goma is installed in the
default location (~/goma), it will work out-of-the-box with the `mojob.py gn`,
`mojob.py build` workflow described above.

You can also manually add:
```
use_goma = true
```

at the end of the file opened through:
```
$ gn args out/Debug
```

After you close the editor `gn gen out/Debug` will run automatically. Now you
can dramatically increase the number of parallel tasks:
```
$ ninja -C out/Debug -j 1000
```

### Official builds

Official builds for android generate a signed Mojo Shell intended for
distribution. You normally should not need to produce one. If you have any
questions, reach out to [etiennej@chromium.org](mailto:etiennej@chromium.org).

## Update your checkout

You can update your checkout like this. The order is important. You must do the
`git pull` first because `gclient sync` is dependent on the current revision.
```
# Fetch changes from upstream and rebase the current branch on top
$ git pull --rebase
# Update all modules as directed by the DEPS file
$ gclient sync
```

You do not need to rerun `gn gen out/Debug` - ninja does so automatically each
time you build. You might need to rerun `mojo/tools/mojob.py gn` if the GN
flags have changed.

## Run Mojo Shell

Devtools `mojo_run` is a universal shell runner abstracting away the differences
between running on Linux and Android.

Having built Mojo as described above, a demo app can be run as follows:

```
mojo/devtools/common/mojo_run https://core.mojoapps.io/spinning_cube.mojo  # Linux
mojo/devtools/common/mojo_run https://core.mojoapps.io/spinning_cube.mojo --android  # Android
```

### Development server

Whenever `mojo_run` is run, a development server is set up according to [the
config file](mojoconfig). The server runs on your machine, serving the locally
built apps, but appears to the shell under the `https://core.mojoapps.io` host.

You can ignore the config file and skip spawning the local server (for example,
in order to use apps at the actual https://core.mojoapps.io web host) by passing
`--no-config-file` to `mojo_run`.

### More examples

Some applications can be run directly from the source tree. The development
server serves the `src` directory, allowing to refer to these apps. For
instance, this command serves a dart Mojo app from the source at
`examples/dart/device_info/main.dart`:

```sh
mojo_run https://core.mojoapps.io/examples/dart/device_info/main.dart [--android]
```

Some applications are meant to be run embedded in a **window manager**. To run
these, you can pass the app url using the `--embed` flag. This will run the
window manager and pass the given url to it:

```sh
devtools/common/mojo_run --embed https://core.mojoapps.io/ganesh_app.mojo [--android]
```

By default, `mojo_run` uses https://core.mojoapps.io/kiosk_wm.mojo as the window
manager. You can pass a different window manager url using the
`--window-manager` flag to override this.

For additional information on `mojo_run` refer to the built-in help and the
[documentation](https://github.com/domokit/devtools/blob/master/README.md#runner).
You can also request more information on what the tool is doing for you by
passing the `--verbose` flag.

### <a name="debugging"></a>Debugging, tracing, profiling

Devtools `mojo_debug` allows you to interactively inspect a running shell,
collect performance traces and attach a gdb debugger.

For additional information refer to the built-in help and the
[documentation](https://github.com/domokit/devtools/blob/master/README.md#debugger).

### Android set-up

#### Adb

For the Android tooling to work, you will need to have `adb` in your PATH. For
that, you can either run:
```
source build/android/envsetup.sh
```

each time you open a fresh terminal, or add something like:
```
export PATH="$PATH":$MOJO_DIR/src/third_party/android_tools/sdk/platform-tools
```

to your ~/.bashrc file, $MOJO_DIR being a path to your Mojo checkout.

#### Device

**The device has to be running Android 5.0 (Lollipop) or newer.**

Many features useful for development (ie. streaming of the shell stdout when
running shell on the device) will not work unless the device is rooted and
running a userdebug build. For Googlers, [follow the instructions at this
link](http://go/mojo-internal-build-instructions).

### Running manually on Linux

If you wish to, you can also run the Linux Mojo shell directly with no wrappers:
```
./out/Debug/mojo_shell out/Debug/spinning_cube.mojo
```

## Contribute

With git you should make all your changes in a local branch. Once your change is
committed, you can delete this branch.

Create a local branch named "mywork" and make changes to it.
```
  cd src
  git new-branch mywork
  vi ...
```
Commit your change locally. (this doesn't commit your change to the SVN or Git
server)

```
  git commit -a
```

Fix your source code formatting.

```
$ git cl format
```

Upload your change for review.

```
$ git cl upload
```

Respond to review comments.

See [Contributing code](http://www.chromium.org/developers/contributing-code)
for more detailed git instructions, including how to update your CL when you get
review comments. There's a short tutorial that might be helpful to try before
making your first change: [C++ in Chromium
101](http://dev.chromium.org/developers/cpp-in-chromium-101-codelab).

To land a change after receiving LGTM:
```
$ git cl land
```

Don't break the build! Waterfall is here:
http://build.chromium.org/p/client.mojo/waterfall
