# Pure C server

## Compiling

You need [libwebsockets](https://libwebsockets.org/) and
[json-c](https://github.com/json-c/json-c) to compile and start
the server. Once you install them, run:

```bash
$ gcc cycle.c -o cycle -lwebsockets
$ ./cycle [PORT]
```

to start the cycle generation server (which starts on PORT). To
start the definitions server, run:

```bash
$ gcc definition.c -o definition -lwebsockets -ljson-c
$ ./definition [PORT]
```

See [the web client implementation](../web) to learn how to use this.
