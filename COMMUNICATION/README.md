# Communication layer status

The C message bus is a bounded, synchronous in-process callback registry. A
publish snapshots the subscribers for that publication; callbacks receive a
borrowed payload valid only until they return. Subscribing or unsubscribing
during callback delivery affects later publications. The bus is not thread-safe
and is not connected to a kernel IPC transport.

RPC uses a required exchange callback and opaque nonzero endpoint handle. It
passes fixed-size request/response records through that backend and preserves
the caller's response if transport fails. Timeouts are expressed in
milliseconds. This is an adapter seam, not an L4 implementation; a
kernel-specific transport must validate capabilities and bind its message
format.

The Go shared-library bridge accepts C function pointers and dispatches to
those callbacks synchronously, with a 64-subscriber bound and explicit
unsubscribe. It is a separate process/runtime surface and does not bridge the
C bus or kernel IPC automatically.

Host checks: `make test-communication`.
