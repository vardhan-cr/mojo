# mojo_run

`mojo_run` allows you to run a Mojo shell either on the host, or on an attached
Android device.

```sh
mojo_run APP_URL  # Run on the host.
mojo_run APP_URL --android  # Run on Android device.
mojo_run "APP_URL APP_ARGUMENTS"  # Run an app with startup arguments
```

Unless running within a Mojo checkout, we need to indicate the path to the shell
binary:

```sh
mojo_run --shell-path path/to/shell/binary APP_URL
```

Some applications are meant to be run embedded in a **window manager**. To run
these, you can pass the app url using the `--embed` flag. This will run the
window manager and pass the given url to it:

```sh
mojo_run --embed APP_URL [--android]
```

By default, `mojo_run` uses https://core.mojoapps.io/kiosk_wm.mojo as the window
manager. You can pass a different window manager url using the
`--window-manager` flag to override this.
