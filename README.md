Mojo
====

Mojo is an effort to extract a common platform out of Chrome's renderer and plugin processes that can support multiple types of sandboxed content, such as HTML, Pepper, or NaCl.

## Set up your environment

The instructions below only need to be done once. Note that a simple "git clone" command is not sufficient to build the source code because this repo uses the gclient command from depot_tools to manage most third party dependencies.

1. Download depot_tools and make sure it is in your path:<br>http://www.chromium.org/developers/how-tos/install-depot-tools<br>

2. [Googlers only] Install Goma in ~/goma.

3. Create a directory somewhere for your checkout (preferably on an SSD), cd into it, and run the following commands:


```
$ fetch mojo # append --target_os=android to include Android build support.
$ cd src
$ ./build/install-build-deps.sh
$ mojo/tools/mojob.py gn
```

The "fetch mojo" command does the following:
- creates a directory called 'src' under your checkout directory
- clones the repository using git clone
- clones dependencies with gclient sync

`install-build-deps.sh` installs any packages needed to build, then `mojo/tools/mojob.py gn` runs `gn args` and configures the build directory, out/Debug.

If the fetch command fails, you will need to delete the src directory and start over.

## Build Mojo

Build Mojo by running:

```
$ ninja -C out/Debug -j 10
```

(If you are a Googler, see the section at the end of this document for faster builds.)

You can also use the mojob.py script for building. This script automatically calls ninja and sets -j to an appropriate value based on whether Goma is present. You cannot specify a target name with this script.
```
mojo/tools/mojob.py build
```

Run a demo:
```
mojo/tools/mojo_demo.py --browser
```

Run the tests:
```
mojo/tools/mojob.py test
```

## Update your repo

You can update your repo like this:
```
$ gclient sync
$ git pull --rebase
```

You do not need to rerun "gn gen out/Debug". Ninja will do so automatically as needed.

## Contribute

With git you should make all your changes in a local branch. Once your change is committed, you can delete this branch.

Create a local branch named "mywork" and make changes to it.
```
  cd src
  git new-branch mywork
  vi ...
```
Commit your change locally (this doesn't commit your change to the SVN or Git server)

```
  git commit -a
```

Fix your source code formatting

```
$ git cl format
```

Upload your change for review

```
$ git cl upload
```

Respond to review comments

See <a href="http://www.chromium.org/developers/contributing-code">Contributing code</a> for more detailed git instructions, including how to update your CL when you get review comments. There's a short tutorial that might be helpful to try before your first change: <a href="http://dev.chromium.org/developers/cpp-in-chromium-101-codelab">C++ in Chromium 101</a>.

To land a change after receiving LGTM:
```
$ git cl land
```

Don't break the build! Waterfall is here: http://build.chromium.org/p/client.mojo/waterfall

## Android Builds

To build for Android, first make sure you've downloaded build support for Android, which you would have done by adding --target_os=android when you ran `fetch mojo`. If you didn't do that, there's an easy fix. Edit the file .gclient in your root Mojo directory (the parent directory to src.) Add this line at the end of the file:

```
target_os = [u'android']
```

Pull down all of the packages with this command:

```
$ gclient sync
```

Prepare the build directory for Android:

```
$ src/mojo/tools/mojob.py gn --android
```

Finally, perform the build. The result will be in out/android_Debug:

```
$ src/mojo/tools/mojob.py build --android
```

If you see javac compile errors, make sure you have an up-to-date JDK:
https://code.google.com/p/chromium/wiki/AndroidBuildInstructions#Install_Java_JDK

## Googlers

If you're a Googler, you can use Goma, a distributed compiler service for open-source projects such as Chrome and Android. The instructions below assume that Goma is installed in the default location (~/goma).

To enable Goma, update your "args.gn" file. Open the file in your editor with this command:
```
$ gn args out/Debug
```

Add this line to the end of the file:
```
use_goma = true
```

After you close the editor, the "gn args" command will automatically run "gn gen out/Debug" again.

Now you can dramatically increase the number of parallel tasks:
```
$ ninja -C out/Debug -j 1000
```

## Run Mojo Shell

### On Android

0. Prerequisites:
    * Before you start, you'll need a device with an unlocked bootloader, otherwise you won't be able to run adb root (or any of the commands that require root). For Googlers, <a href="http://go/mojo-internal-build-instructions">follow this link</a> and follow the instructions before returning to this page.
    * Ensure your device is running Lollipop and has an userdebug build.

1. Build changed files:
    ```
    mojob.py build --android
    ```

2. Push mojo_shell to to your device:
    ```
    adb install -r out/android_Debug/apks/MojoShell.apk
    ```

3. Run Mojo Shell on the device:
    ```
    ./mojo/tools/android_mojo_shell.py mojo:sample_app
    ```

4. If you get a crash you won't see symbols. Use tools/android_stack_parser/stack to map back to symbols, e.g.:
    ```
    adb logcat | ./tools/android_stack_parser/stack
    ```

### On Linux

1. Build the mojo target, which builds the shell and all examples:
    ```
    ninja -C out/Release mojo
    ```

2. Run Mojo Shell:
    ```
    ./out/Release/mojo_shell mojo:sample_app
    ```

3. Optional: Run Mojo Shell with an HTTP server
    ```
    cd out/Release
    python -m SimpleHTTPServer 4444 &
    ./mojo_shell --origin=http://127.0.0.1:4444 --disable-cache mojo:mojo_sample_app
    ```
