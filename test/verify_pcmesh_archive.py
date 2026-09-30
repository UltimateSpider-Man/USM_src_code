"""Exercise the native ZIP reader with independently generated ZIP fixtures.

Usage: python test/verify_pcmesh_archive.py build-fbx/mod_pcmesh_archive_probe.exe
"""
import io
import pathlib
import struct
import subprocess
import sys
import tempfile
import warnings
import zipfile


PROBE = pathlib.Path(sys.argv[1]).resolve()
PAYLOAD = b"PCM " + struct.pack("<I", 0x601) + bytes(range(256)) * 16


def archive(method=zipfile.ZIP_DEFLATED, name="nested/VENOM.PCMESH"):
    output = io.BytesIO()
    with zipfile.ZipFile(output, "w", compression=method) as stream:
        stream.writestr(name, PAYLOAD)
    return bytearray(output.getvalue())


def change(data, offset, fmt, value):
    result = bytearray(data)
    struct.pack_into(fmt, result, offset, value)
    return result


def directory(data):
    return data.index(b"PK\x01\x02")


with tempfile.TemporaryDirectory(prefix="usm-pcmesh-zip-") as temp:
    passed = 0

    def check(label, data, expected, stem="vEnOm"):
        global passed
        path = pathlib.Path(temp) / (label + ".zip")
        path.write_bytes(data)
        result = subprocess.run([str(PROBE), str(path), stem], capture_output=True, text=True)
        assert (result.returncode == 0) == expected, (label, result.returncode, result.stdout, result.stderr)
        if expected:
            assert f"bytes={len(PAYLOAD)} version=0x601" in result.stdout, (label, result.stdout)
        passed += 1

    compressed = archive()
    stored = archive(zipfile.ZIP_STORED)
    cd = directory(compressed)
    eo = compressed.index(b"PK\x05\x06")
    check("deflated", compressed, True)
    check("stored", stored, True)
    check("extension_and_path", compressed, True, "extra/venom.pcmesh")
    check("missing_member", compressed, False, "absent")
    check("empty", b"", False)
    check("truncated_end", compressed[:-1], False)
    check("truncated_local", compressed[:20], False)
    check("bad_directory_extent", change(compressed, eo + 16, "<I", len(compressed)), False)
    check("bad_directory_count", change(compressed, eo + 10, "<H", 3), False)
    check("zip64", change(compressed, eo + 16, "<I", 0xFFFFFFFF), False)
    check("bad_local_offset", change(compressed, cd + 42, "<I", cd), False)
    check("bad_local_size", change(compressed, 18, "<I", 1), False)
    check("bad_local_name", change(compressed, 30, "B", ord("x")), False)
    check("bad_compressed_extent", change(compressed, cd + 20, "<I", len(compressed)), False)
    check("allocation_limit", change(compressed, cd + 24, "<I", 64 * 1024 * 1024 + 1), False)
    check("encrypted", change(compressed, cd + 8, "<H", 1), False)
    check("unsupported_compression", change(compressed, cd + 10, "<H", 99), False)
    bad_crc = change(compressed, cd + 16, "<I", 0)
    bad_crc = change(bad_crc, 14, "<I", 0)
    check("crc_mismatch", bad_crc, False)
    short_output = change(compressed, cd + 24, "<I", 8)
    short_output = change(short_output, 22, "<I", 8)
    check("deflate_output_limit", short_output, False)
    long_output = change(compressed, cd + 24, "<I", len(PAYLOAD) + 1)
    long_output = change(long_output, 22, "<I", len(PAYLOAD) + 1)
    check("deflate_output_length", long_output, False)
    payload_offset = 30 + struct.unpack_from("<H", stored, 26)[0]
    corrupt = change(stored, payload_offset + 40, "B", 255)
    check("stored_payload_corrupt", corrupt, False)

    duplicate = io.BytesIO()
    with warnings.catch_warnings(), zipfile.ZipFile(duplicate, "w") as stream:
        warnings.simplefilter("ignore")
        stream.writestr("one/VENOM.PCMESH", PAYLOAD)
        stream.writestr("two/venom.pcmesh", PAYLOAD)
    check("ambiguous_stem", duplicate.getvalue(), False)

    class Unseekable(io.BytesIO):
        def seekable(self):
            return False

        def seek(self, *args):
            raise io.UnsupportedOperation("stream")

    streamed = Unseekable()
    with zipfile.ZipFile(streamed, "w", compression=zipfile.ZIP_DEFLATED) as stream:
        stream.writestr("VENOM.PCMESH", PAYLOAD)
    check("data_descriptor", streamed.getvalue(), True)

    comment = io.BytesIO()
    with zipfile.ZipFile(comment, "w", compression=zipfile.ZIP_DEFLATED) as stream:
        stream.writestr("VENOM.PCMESH", PAYLOAD)
        stream.comment = b"comment containing PK\x05\x06 but no valid end record"
    check("archive_comment", comment.getvalue(), True)

print(f"PASS: {passed} native PCMESH ZIP checks")
