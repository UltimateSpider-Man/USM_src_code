"""Three separate processes, actual TCP/UDP, plus malformed traffic.
Local OS evidence only. No game/hardware/Radmin certificate is implied.
"""
import pathlib
import socket
import struct
import subprocess
import sys
import tempfile
import time

exe = str(pathlib.Path(sys.argv[1]).resolve())
processes = []
files = []
temporary = tempfile.TemporaryDirectory(prefix="usm-online-")
try:
    root = pathlib.Path(temporary.name)
    def launch(name, *arguments):
        log = root / (name + ".log")
        stream = log.open("w", encoding="utf-8")
        files.append(stream)
        p = subprocess.Popen([exe, *arguments], stdout=stream, stderr=subprocess.STDOUT)
        processes.append(p)
        return p, log
    host, host_log = launch("host", "host", "127.0.0.1", "0", "Host", "2.5", "3")
    deadline = time.monotonic() + 4
    port = None
    while time.monotonic() < deadline:
        lines = host_log.read_text(encoding="utf-8").splitlines()
        ready = next((line for line in lines if line.startswith("READY ")), None)
        if ready is not None:
            port = int(ready.split()[1]); break
        if host.poll() is not None:
            raise RuntimeError(host_log.read_text(encoding="utf-8"))
        time.sleep(.01)
    if not port:
        raise RuntimeError("host did not open a port")
    # Invalid lengths and control messages must be rejected without taking
    # down the server, allocating arbitrary buffers or filling all seats.
    for payload in (b"\xff\xff", b"\x00\x01x", b"\x00\x07garbage"):
        with socket.create_connection(("127.0.0.1", port), timeout=1) as attacker:
            attacker.sendall(payload)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
        for payload in (b"x", bytes(1201), b"USMO" + bytes(40), bytes(65507)):
            udp.sendto(payload, ("127.0.0.1", port))
    a, a_log = launch("alice", "join", "127.0.0.1", str(port), "Alice", "1.7", "3")
    b, b_log = launch("bob", "join", "127.0.0.1", str(port), "Bob", "2.0", "3")
    for name, p, log in (("Alice", a, a_log), ("Bob", b, b_log), ("Host", host, host_log)):
        code = p.wait(timeout=8)
        print(name + ":\n" + log.read_text(encoding="utf-8"), end="")
        if code != 0:
            raise RuntimeError(f"{name} exited {code}")
    print("3-process TCP+UDP/invalid-traffic test passed")
finally:
    for p in processes:
        if p.poll() is None:
            p.terminate()
            try: p.wait(timeout=2)
            except subprocess.TimeoutExpired:
                p.kill(); p.wait()
    for stream in files:
        stream.close()
    temporary.cleanup()
