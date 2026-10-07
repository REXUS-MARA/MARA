#!/usr/bin/env python3
"""Fake platform motor drive for the MARA integration tests. Runs on the Pi (needs sudo).

Creates a pseudo-terminal, links it at the motor UART path the FSW opens (MotorUartDevice,
under /dev/serial/by-id/), and decodes the Waveshare USB-CAN-A frames the FSW sends:

    AA, type (0b11 E R DLC), CAN ID (2 bytes LE, 4 if extended), DLC data bytes, 55

Each frame is appended to the log as one JSON line. CANopen SDO downloads (COB-ID 0x600+node)
are decoded into index, subindex and value. Stopping the script (SIGTERM/SIGINT) removes the
link and closes the pty, which the FSW sees as the adapter being unplugged.

Standard library only, so it runs on a stock Raspberry Pi OS.
"""

import argparse
import json
import os
import select
import signal
import sys
import time
import tty


def decode_sdo(cob_id, data):
    """Decode a CANopen expedited SDO download request, or return {}."""
    if not 0x600 <= cob_id <= 0x67F or len(data) != 8:
        return {}
    size = {0x2F: 1, 0x2B: 2, 0x27: 3, 0x23: 4}.get(data[0])
    if size is None:
        return {}
    return {
        "node": cob_id - 0x600,
        "index": data[1] | (data[2] << 8),
        "subindex": data[3],
        "value": int.from_bytes(bytes(data[4 : 4 + size]), "little", signed=False),
        "size": size,
    }


def frames(stream):
    """Yield (cob_id, data) for each complete frame; keep partial frames for the next call."""
    while True:
        try:
            start = stream.index(0xAA)
        except ValueError:
            stream.clear()
            return
        del stream[:start]
        if len(stream) < 2:
            return
        frame_type = stream[1]
        if frame_type & 0xC0 != 0xC0:  # not a valid type byte: resync
            del stream[0]
            continue
        id_len = 4 if frame_type & 0x20 else 2
        dlc = frame_type & 0x0F
        total = 2 + id_len + dlc + 1
        if len(stream) < total:
            return
        if stream[total - 1] != 0x55:  # bad end code: resync
            del stream[0]
            continue
        cob_id = int.from_bytes(bytes(stream[2 : 2 + id_len]), "little")
        data = list(stream[2 + id_len : 2 + id_len + dlc])
        del stream[:total]
        yield cob_id, data


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--link", required=True, help="path the FSW opens (MotorUartDevice)")
    parser.add_argument("--log", required=True, help="JSON-lines file to append frames to")
    args = parser.parse_args()

    master, slave = os.openpty()
    tty.setraw(slave)
    slave_path = os.ttyname(slave)
    os.makedirs(os.path.dirname(args.link), exist_ok=True)
    if os.path.lexists(args.link):
        os.unlink(args.link)
    os.symlink(slave_path, args.link)
    os.close(slave)  # the FSW opens it by path

    def stop(*_):
        if os.path.islink(args.link):
            os.unlink(args.link)
        os.close(master)  # hangs up the FSW's side
        sys.exit(0)

    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGINT, stop)
    print(f"fake drive: {args.link} -> {slave_path}", flush=True)

    stream = bytearray()
    with open(args.log, "a", buffering=1) as log:
        while True:
            readable, _, _ = select.select([master], [], [], 1.0)
            if not readable:
                continue
            try:
                chunk = os.read(master, 4096)
            except OSError:  # no one has the slave open yet
                time.sleep(0.1)
                continue
            stream.extend(chunk)
            for cob_id, data in frames(stream):
                entry = {"t": time.time(), "cob_id": cob_id, "data": [f"{b:02X}" for b in data]}
                entry.update(decode_sdo(cob_id, data))
                log.write(json.dumps(entry) + "\n")


if __name__ == "__main__":
    main()
