# BAURA System API/ABI v2 status

`API_ABI_v2.json` and `baura_abi_v2.h` define the BAURA IPC wire contract for
version 2.0. The JSON is the semantic and field-layout source of truth; the C
header exposes stable values and IDs to Core consumers.

The native BAURA target is PC x86_64 (little-endian, 64-bit pointers). Wire
fields use explicit fixed widths and byte-wise encoding, so host-side codec
tests do not depend on the host pointer width. `BAURA_ABI_DEFAULT` remains
unbound to a compiler-specific x86_64 calling convention until the target
toolchain and userspace ABI are selected.

## Normative wire rules

- A frame is a 32-byte header followed by exactly `payload_bytes` bytes. Integer
  fields are fixed-width little-endian; signed values use two's complement.
  Payload fields are tightly concatenated in the listed order with no padding.
  Do not serialize native C structs, native pointers, or compiler padding.
- Header offsets are: magic `u32` at 0 (`BAUR` on the wire), major `u16` at 4,
  minor `u16` at 6, kind `u16` at 8, flags `u16` at 10, opcode `u32` at 12,
  request ID `u64` at 16, payload length `u32` at 24, attachment count `u16`
  at 28, and reserved-zero `u16` at 30. The maximum payload is 16384 bytes.
  V2 does not fragment frames.
- Kinds are REQUEST=1, RESPONSE=2, COMPLETION=3. Flags and reserved fields
  must be zero. Request IDs are nonzero and unique per client while outstanding.
  Responses echo both opcode and request ID.
- Every response payload begins with `u32 status, u32 error`. SUCCESS is `0`,
  FAILURE is `1`, PENDING is `2`. A failed response has a nonzero error and no
  result. Successful and pending responses have error zero. PENDING is only
  valid for non-blocking operations and is followed by one terminal COMPLETION
  with the same opcode/request ID; a completion cannot itself be pending.
  FAILURE and PENDING responses carry no attachments. Successful responses
  carry exactly the attachments specified for that operation.
- The stable error values are `0x1001` through `0x100E` in the order declared
  in `baura_abi_v2.h`. These assignments were absent from the supplied JSON;
  this integration assigns and versions them as BAURA ABI v2.0 values.
  `BAURA_ABI_ERR_*` in C corresponds to the `BAURA_ERR_*` wire names.
- Strings are UTF-8 encoded as `u32 byte_length` followed by exactly that many
  bytes (no NUL). Paths are canonical `PC/<drive>:/<path>` and are limited to
  255 bytes, consistent with the current bounded VFS path representation.
  Other strings are limited to 4096 bytes.
- A shared-memory descriptor is `u16 attachment_ordinal, u16 reserved_zero,
  u64 offset, u64 length`. Attachments are transferred kernel capabilities,
  not integer values that grant authority. The receiver validates object type,
  rights, range, and lifetime; all range arithmetic must be overflow-safe.
  `BAURA_BUFFER` never means a process-local pointer on the wire.
- The caller needs the service endpoint capability and the operation-specific
  capability listed in the contract. The receiving service authorizes before
  side effects. Capability transfer and endpoint binding are supplied by the
  selected L4 transport, not encoded as pointers in the frame. The attachment
  count must exactly match the operation's requirement; extra attachments are
  rejected.
- Unknown versions, kinds, opcodes, nonzero reserved bits, invalid lengths,
  attachments, or operation payloads are rejected before side effects.

## V2.0 reference opcodes and payloads

| Operation | Opcode | Request payload | Successful result |
| --- | ---: | --- | --- |
| FS-0001 `BAURA_FS_CREATE` | `0x00010001` | path string, `u64 options` | `u64 file_handle` |
| FS-0002 `BAURA_FS_READ` | `0x00010002` | `u64 file_handle`, `u64 size`, `i64 offset`, shared-memory descriptor | `u64 bytes_read` |
| PR-0001 `BAURA_PROC_CREATE` | `0x00020001` | image path string, arguments string (zero length means absent), `u64 options` | `u64 process_id` |
| MEM-0001 `BAURA_MEM_ALLOC` | `0x00030001` | `u64 size`, `u64 flags` | `u64 allocated_bytes` plus one shared-memory capability |
| NET-0001 `BAURA_NET_CONNECT` | `0x00040001` | endpoint string, `u64 options` | empty result |

Attachment counts are direction-specific: FS-0002 has one request attachment
and none in its response; MEM-0001 has none in its request and one in its
response. All other reference operations have no attachments.

FS create accepts options zero or `BAURA_FS_CREATE_NEW` for exclusive creation,
or `BAURA_FS_CREATE_ALWAYS` to create or replace/truncate. The two named bits
cannot be combined; unknown bits are invalid. FS read offset `-1` means use and
advance the service-maintained cursor; nonnegative offsets are positioned
reads and do not change that cursor; values below `-1` are invalid. A short read
is successful and returns the number of bytes read. Process-create options,
memory flags, and network options must be zero in v2.0.

For FS read, the client transfers one shared-memory capability writable by the
filesystem service, and the requested nonzero size must fit from `buffer_offset`.
Memory allocate returns a capability with caller read/write access; its object
size must equal the returned `allocated_bytes` and be at least the requested
size. The details for each operation, including required capabilities and
blocking behavior, are also recorded in the JSON.

## Implementation status and remaining binding

The codec is architecture-neutral C protocol code intended for the x86_64
target; host tests do not validate seL4 hardware integration. The constants and
wire layouts are specified; the OS does **not** yet provide
callable v2 IPC stubs or service dispatchers. The repository's existing default boot target is still a 32-bit Multiboot
bring-up image; it does not start seL4 or implement
the filesystem, isolated process creation, memory-service, or network-service
backends. This specification deliberately does not claim a selected seL4
image format, BootInfo contract, endpoint bootstrap mechanism, or transport
message-register binding. Those must be confirmed against the actual seL4
x86_64 configuration and documented as a transport binding before calls can
be considered operational.

`make baura-core-x86_64` builds a freestanding static archive with the x86_64
cross compiler. `make test-core-x86_64` runs the C Core suite with a 64-bit
host ABI. `baura_core_initial_task_start` is a platform-neutral Core entry
point that requires a boot-info validator, initializes the Core only after
validation succeeds, and reports readiness through a caller-supplied console
callback. Neither target currently links the archive into seL4 or an EFI/GRUB
image.

The Core exposes task lifecycle and authority-check backend contracts for a
platform adapter. Process launch returns `BAURA_ERR_INVALID_STATE` until a
complete task backend is installed. Capability authorization likewise fails
closed when no authority checker is installed. The initial-task entry also
requires a supported versioned platform structure, an opaque boot-info object,
and a validator; validation must succeed before Core initialization. These
contracts do not themselves create seL4 tasks, validate seL4 capabilities, or
define a kernel-specific BootInfo layout. There is no concrete seL4 adapter or
IPC transport in the current build.

The C VFS path resolver uses the registered volume manager for drive-name
resolution. It normalizes repeated separators and `.` components and rejects
`..` traversal. This connects logical paths to registered volumes, not to a
filesystem: mounted filesystem operations and physical storage drivers remain
unimplemented.

The checked-in seL4 tree (version 16.0.0-dev) declares the `pc99` platform for
both `ia32` and `x86_64`. `make sel4-kernel-x86_64` explicitly configures
`PLATFORM=pc99`, `KernelSel4Arch=x86_64`, and kernel printing, then builds the seL4
`kernel.elf` target. This validates/configures only the kernel build, not that
its ELF is compatible with GRUB or that BAURA Core has been started. Toolchain
selection is provided to CMake through `SEL4_CMAKE_ARGS`; supported host tools
and additional seL4 build dependencies must be installed separately.

The v2.0 C scalar alias for `BauraBuffer` is only an API-side opaque pointer
placeholder; wire calls use the shared-memory attachment contract above. The
`BAURA_ABI_DEFAULT` macro currently leaves the platform C default in effect and
does not promise a cross-platform register/stack calling convention. The
catalogue target of 100,000 operations is not an implemented or specified
catalogue: this source defines five reference operations.
