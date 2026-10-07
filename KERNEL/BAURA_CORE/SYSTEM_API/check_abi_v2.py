#!/usr/bin/env python3
"""Check that the public C ABI constants match the normative JSON contract."""

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
SPEC_PATH = ROOT / "KERNEL" / "BAURA_CORE" / "SYSTEM_API" / "API_ABI_v2.json"
HEADER_PATH = ROOT / "KERNEL" / "BAURA_CORE" / "SYSTEM_API" / "baura_abi_v2.h"

OPERATION_ID_TO_OPCODE = {
    "FS-0001": "BAURA_ABI_OPCODE_FS_CREATE",
    "FS-0002": "BAURA_ABI_OPCODE_FS_READ",
    "PR-0001": "BAURA_ABI_OPCODE_PROCESS_CREATE",
    "MEM-0001": "BAURA_ABI_OPCODE_MEMORY_ALLOCATE",
    "NET-0001": "BAURA_ABI_OPCODE_NETWORK_CONNECT",
}

OPERATION_ID_TO_SYMBOL = {
    "FS-0001": "BAURA_ABI_OP_FS_CREATE",
    "FS-0002": "BAURA_ABI_OP_FS_READ",
    "PR-0001": "BAURA_ABI_OP_PROCESS_CREATE",
    "MEM-0001": "BAURA_ABI_OP_MEMORY_ALLOCATE",
    "NET-0001": "BAURA_ABI_OP_NETWORK_CONNECT",
}

REQUEST_ATTACHMENT_COUNTS = {
    "FS-0001": 0,
    "FS-0002": 1,
    "PR-0001": 0,
    "MEM-0001": 0,
    "NET-0001": 0,
}

RESPONSE_ATTACHMENT_COUNTS = {
    "FS-0001": 0,
    "FS-0002": 0,
    "PR-0001": 0,
    "MEM-0001": 1,
    "NET-0001": 0,
}

NON_BLOCKING_OPERATIONS = {"FS-0001", "PR-0001", "MEM-0001"}

ERROR_ID_TO_MACRO = {
    "BAURA_ERR_INVALID_ARGUMENT": "BAURA_ABI_ERR_INVALID_ARGUMENT",
    "BAURA_ERR_INVALID_PATH": "BAURA_ABI_ERR_INVALID_PATH",
    "BAURA_ERR_NOT_FOUND": "BAURA_ABI_ERR_NOT_FOUND",
    "BAURA_ERR_ALREADY_EXISTS": "BAURA_ABI_ERR_ALREADY_EXISTS",
    "BAURA_ERR_PERMISSION": "BAURA_ABI_ERR_PERMISSION",
    "BAURA_ERR_CAPABILITY": "BAURA_ABI_ERR_CAPABILITY",
    "BAURA_ERR_UNAVAILABLE": "BAURA_ABI_ERR_UNAVAILABLE",
    "BAURA_ERR_TIMEOUT": "BAURA_ABI_ERR_TIMEOUT",
    "BAURA_ERR_NO_MEMORY": "BAURA_ABI_ERR_NO_MEMORY",
    "BAURA_ERR_IO": "BAURA_ABI_ERR_IO",
    "BAURA_ERR_INVALID_HANDLE": "BAURA_ABI_ERR_INVALID_HANDLE",
    "BAURA_ERR_NOT_SUPPORTED": "BAURA_ABI_ERR_NOT_SUPPORTED",
    "BAURA_ERR_SECURITY": "BAURA_ABI_ERR_SECURITY",
    "BAURA_ERR_INTERNAL": "BAURA_ABI_ERR_INTERNAL",
}


def fail(message: str) -> None:
    print(f"ABI v2 consistency check failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def parse_header_macros(header: str) -> dict[str, str]:
    return dict(
        re.findall(
            r"^[ \t]*#define[ \t]+(\w+)[ \t]+([^\r\n]+?)[ \t]*$",
            header,
            re.MULTILINE,
        )
    )


def parse_integer(value: str) -> int:
    cleaned = value.strip().rstrip("uUlL")
    wrapper = re.fullmatch(r"UINT(?:8|16|32|64)_C\((.+)\)", cleaned)
    if wrapper is not None:
        cleaned = wrapper.group(1).strip()
    return int(cleaned, 0)


def main() -> None:
    try:
        specification = json.loads(SPEC_PATH.read_text(encoding="utf-8"))
        header = HEADER_PATH.read_text(encoding="utf-8")
    except (OSError, json.JSONDecodeError) as error:
        fail(str(error))

    if specification.get("specification", {}).get("version") != "2.0":
        fail("the JSON specification version is not 2.0")

    wire = specification.get("ipc_protocol", {})
    header_spec = wire.get("header", {})
    fields = {field.get("name"): field for field in header_spec.get("fields", [])}
    offsets = {
        "magic": "BAURA_IPC_MAGIC_OFFSET",
        "major": "BAURA_IPC_MAJOR_OFFSET",
        "minor": "BAURA_IPC_MINOR_OFFSET",
        "kind": "BAURA_IPC_KIND_OFFSET",
        "flags": "BAURA_IPC_FLAGS_OFFSET",
        "opcode": "BAURA_IPC_OPCODE_OFFSET",
        "request_id": "BAURA_IPC_REQUEST_ID_OFFSET",
        "payload_bytes": "BAURA_IPC_PAYLOAD_SIZE_OFFSET",
        "attachment_count": "BAURA_IPC_ATTACHMENT_COUNT_OFFSET",
        "reserved": "BAURA_IPC_RESERVED_OFFSET",
    }
    macros = parse_header_macros(header)
    for field_name, macro in offsets.items():
        field = fields.get(field_name)
        if field is None or macro not in macros:
            fail(f"missing normative/header field {field_name}")
        if parse_integer(macros[macro]) != field.get("offset"):
            fail(f"{macro} does not match JSON offset {field.get('offset')}")

    if parse_integer(macros.get("BAURA_IPC_HEADER_SIZE", "0")) != header_spec.get(
        "size_bytes"
    ):
        fail("BAURA_IPC_HEADER_SIZE differs from the JSON header size")
    if parse_integer(macros.get("BAURA_IPC_MAX_PAYLOAD", "0")) != wire.get(
        "framing", {}
    ).get("maximum_payload_bytes"):
        fail("BAURA_IPC_MAX_PAYLOAD differs from the JSON framing limit")
    for field_name, macro_name in (
        ("flags", "BAURA_IPC_FLAGS_VALUE"),
        ("reserved", "BAURA_IPC_RESERVED_VALUE"),
    ):
        field = fields.get(field_name)
        if field is None or "value" not in field or macro_name not in macros:
            fail(f"missing zero-value binding for IPC {field_name}")
        if parse_integer(macros[macro_name]) != int(field["value"]):
            fail(f"{macro_name} differs from the JSON {field_name} value")
    magic = next(
        (field.get("value") for field in header_spec.get("fields", [])
         if field.get("name") == "magic"),
        None,
    )
    if magic is None or parse_integer(macros.get("BAURA_IPC_MAGIC", "0")) != int(
        magic, 0
    ):
        fail("BAURA_IPC_MAGIC differs from the JSON protocol magic")
    if parse_integer(macros.get("BAURA_ABI_VERSION_MAJOR", "0")) != int(
        specification["specification"]["version"].split(".")[0]
    ):
        fail("C header major version differs from JSON")
    if parse_integer(macros.get("BAURA_ABI_VERSION_MINOR", "0")) != int(
        specification["specification"]["version"].split(".")[1]
    ):
        fail("C header minor version differs from JSON")
    kinds = fields["kind"].get("values", {})
    for kind_name, macro in (
        ("REQUEST", "BAURA_IPC_KIND_REQUEST"),
        ("RESPONSE", "BAURA_IPC_KIND_RESPONSE"),
        ("COMPLETION", "BAURA_IPC_KIND_COMPLETION"),
    ):
        if kind_name not in kinds or macro not in macros:
            fail(f"missing IPC kind binding for {kind_name}")
        if parse_integer(macros[macro]) != kinds[kind_name]:
            fail(f"{kind_name} kind differs between JSON and C")

    response_fields = wire.get("response_payload", {}).get("fields", [])
    response_status = next(
        (field for field in response_fields if field.get("name") == "status"),
        None,
    )
    if response_status is None:
        fail("missing response status values in JSON")
    statuses = response_status.get("values", {})
    public_statuses = wire.get("status_values", {})
    for status_name, public_name, macro in (
        ("SUCCESS", "BAURA_STATUS_SUCCESS", "BAURA_ABI_STATUS_SUCCESS"),
        ("FAILURE", "BAURA_STATUS_FAILURE", "BAURA_ABI_STATUS_FAILURE"),
        ("PENDING", "BAURA_STATUS_PENDING", "BAURA_ABI_STATUS_PENDING"),
    ):
        match = re.search(rf"\b{re.escape(macro)}\s*=\s*(\d+)", header)
        if (
            status_name not in statuses
            or public_name not in public_statuses
            or match is None
        ):
            fail(f"missing ABI status binding for {status_name}")
        status_value = statuses[status_name]
        if public_statuses[public_name] != status_value:
            fail(f"{public_name} differs within the JSON contract")
        if int(match.group(1)) != status_value:
            fail(f"{status_name} status differs between JSON and C")

    operations = specification.get("reference_implementable_operations", [])
    operation_ids = [operation.get("id") for operation in operations]
    if len(operation_ids) != len(set(operation_ids)):
        fail("duplicate reference operation ID")
    if set(operation_ids) != set(OPERATION_ID_TO_OPCODE):
        fail("reference operation set differs from the checked C binding set")
    protocol_opcodes = wire.get("operation_opcodes", {})
    if set(protocol_opcodes) != set(operation_ids):
        fail("wire opcode table differs from the reference operation set")

    for operation in operations:
        operation_id = operation["id"]
        abi = operation.get("abi", {})
        opcode_macro = OPERATION_ID_TO_OPCODE[operation_id]
        symbol_macro = OPERATION_ID_TO_SYMBOL[operation_id]
        c_symbol_macro = "BAURA_ABI_SYMBOL_" + opcode_macro.replace(
            "BAURA_ABI_OPCODE_", "", 1
        )
        if (
            opcode_macro not in macros
            or symbol_macro not in macros
            or c_symbol_macro not in macros
        ):
            fail(f"{operation_id} is missing its public C binding")
        operation_opcode = int(abi.get("opcode", "0"), 0)
        if parse_integer(macros[opcode_macro]) != operation_opcode:
            fail(f"{operation_id} opcode differs between JSON and C")
        if protocol_opcodes[operation_id] != abi.get("opcode"):
            fail(f"{operation_id} opcode differs within the JSON contract")
        if macros[symbol_macro].strip('"') != operation_id:
            fail(f"{operation_id} identifier differs between JSON and C")
        if macros[c_symbol_macro].strip('"') != abi.get("symbol"):
            fail(f"{operation_id} ABI symbol differs between JSON and C")
        required = (
            "domain",
            "concept",
            "api",
            "abi",
            "wire_payload",
            "requires",
            "errors",
            "mappings",
            "shell",
            "execution",
            "version",
        )
        missing = [key for key in required if key not in operation]
        if missing:
            fail(f"{operation_id} lacks required contract fields: {', '.join(missing)}")
        if abi.get("mechanism") != "IPC":
            fail(f"{operation_id} does not declare the supported IPC mechanism")
        payload = operation.get("wire_payload", {})
        request_attachments = payload.get("request_attachments")
        response_attachments = payload.get("response_attachments")
        if type(request_attachments) is not int or request_attachments < 0:
            fail(f"{operation_id} has an invalid request attachment count")
        if type(response_attachments) is not int or response_attachments < 0:
            fail(f"{operation_id} has an invalid response attachment count")
        if request_attachments != REQUEST_ATTACHMENT_COUNTS[operation_id]:
            fail(f"{operation_id} request attachments differ from the C codec")
        if response_attachments != RESPONSE_ATTACHMENT_COUNTS[operation_id]:
            fail(f"{operation_id} response attachments differ from the C codec")
        blocking = operation.get("execution", {}).get("blocking")
        if type(blocking) is not bool or blocking != (
            operation_id not in NON_BLOCKING_OPERATIONS
        ):
            fail(f"{operation_id} blocking behavior differs from the C codec")
        unknown_errors = set(operation.get("errors", [])) - set(
            wire.get("error_values", {})
        )
        if unknown_errors:
            fail(f"{operation_id} references undefined errors: {sorted(unknown_errors)}")

    error_values = wire.get("error_values", {})
    if set(error_values) != set(ERROR_ID_TO_MACRO):
        fail("wire error table differs from the checked C binding set")
    for json_name, macro_name in ERROR_ID_TO_MACRO.items():
        if json_name not in error_values or macro_name not in header:
            fail(f"missing error binding for {json_name}")
        match = re.search(
            rf"\b{re.escape(macro_name)}\s*=\s*(0x[0-9A-Fa-f]+|\d+)",
            header,
        )
        if match is None:
            fail(f"could not parse C error value {macro_name}")
        if int(match.group(1), 0) != int(error_values[json_name], 0):
            fail(f"{json_name} error value differs between JSON and C")

    print(
        "ABI v2 consistency OK: "
        f"{len(operations)} reference operations, "
        f"{len(offsets)} header offsets, "
        f"{len(ERROR_ID_TO_MACRO)} error values."
    )


if __name__ == "__main__":
    main()
