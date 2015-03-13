Sample Go application that can be loaded into a mojo shell running on Android.
The application exports a MojoMain entry point for the shell and then makes
a GetTimeTicksNow system call.

Setup instructions

1) Generate the NDK toolchain for the android platform you plan to use.

$ cd mojo/src/third_party/android_tools/ndk
$ NDK_ROOT=$HOME/android/ndk-toolchain
$ ./build/tools/make-standalone-toolchain.sh --platform=android-14 --install-dir=$NDK_ROOT
$ NDK_CC=$NDK_ROOT/bin/arm-linux-androideabi-gcc

2) Download/Install the Go compiler.

$ unset GOBIN GOPATH GOROOT
$ hg clone https://code.google.com/p/go
$ export GOROOT=`pwd`/go
$ cd go/src
$ CC_FOR_TARGET=$NDK_CC GOOS=android GOARCH=arm GOARM=7 ./make.bash
$ ls $GOROOT/bin/go

3) Now, we switch to the Mojo workspace and build the sample application.

$ cd mojo/src
$ build/install-build-deps-android.sh
$ mojo/tools/mojob.py gn --android
This should show an error 'assert(go_build_tool != "")', now run
$ gn args out/android_Debug
And append two lines:
mojo_use_go=true
go_build_tool="<path_to_go_binary>"

$ ninja -C out/android_Debug go_sample_app

To run the app:
1) configure port forwarding 4444 -> localhost:4444 on android device or
   use 10.0.2.2 instead of 127.0.0.1 if you are running an android emulator
2) open new terminal and run
$ cd out/android_Debug
$ python -m SimpleHTTPServer 4444
3) in the previous terminal run
$ mojo/tools/android_mojo_shell.py --url-mappings="mojo:go_sample_app"="http://127.0.0.1:4444/obj/mojo/go/go_sample_app" "mojo:go_sample_app"

More inforamtion about building mojo: https://github.com/domokit/mojo/blob/master/README.md
