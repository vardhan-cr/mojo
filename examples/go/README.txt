Sample Go applications.

echo_client, echo_server - sample applications that talk to each other

http_handler - application that implements HTTP server; uses the mojo HTTP
Server service to handle the HTTP protocol details, and just contains the logic
for handling its registered urls.

http_server - application that implements HTTP server; uses go net stack on top
of the mojo network service.

1) Follow steps from //mojo/go/sample_app/README.txt

To run echo client:
$ mojo/tools/android_mojo_shell.py --enable-multiprocess mojo:go_echo_client

To run http handler:
$ mojo/tools/android_mojo_shell.py --enable-multiprocess mojo:go_http_handler

To run http server:
$ mojo/tools/android_mojo_shell.py --enable-multiprocess mojo:go_http_server
