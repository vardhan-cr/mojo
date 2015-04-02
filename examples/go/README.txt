Sample Go echo application.

Instructions to run on android emulator:

1) Follow steps from //mojo/go/sample_app/README.txt

To run client:
$ mojo/tools/android_mojo_shell.py --url-mappings="mojo:go_echo_client"="http://10.0.2.2:4444/obj/examples/go/go_echo_client","mojo:echo_server"="http://10.0.2.2:4444/echo_server.mojo" "mojo:go_echo_client"

To run server:
$ mojo/tools/android_mojo_shell.py --url-mappings="mojo:echo_server"="http://10.0.2.2:4444/obj/examples/go/go_echo_server","mojo:echo_client"="http://10.0.2.2:4444/echo_client.mojo" "mojo:echo_client"

You can't run two go mojo applications at the same time now as they need separated processes.
