#!/usr/bin/env python3
"""A virtual ground link: a pseudo-terminal for the FSW, a TCP server for the GDS.

Runs the FSW and the GDS on one machine (laptop or dev Pi) without a USB-UART adapter.
The GDS only accepts serial ports pyserial can list, which excludes ptys, so the GDS side is
TCP, the same way Mara/ansible/uart_tcp_bridge.py simulates the RXSM link:

    python3 pty_link.py                                  # FSW side: /tmp/mara_fsw_uart, GDS side: TCP 50000
    ./Mara_MaraRPiUART -d /tmp/mara_fsw_uart -b 115200
    fprime-gds -n --dictionary <dict> --communication-selection ip \
        --ip-client --ip-address 127.0.0.1 --ip-port 50000

Standard library only; Ctrl-C to stop.
"""

import argparse
import os
import select
import signal
import socket
import sys
import tty


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--fsw-link", default="/tmp/mara_fsw_uart", help="link for the FSW's -d argument")
    parser.add_argument("--port", type=int, default=50000, help="TCP port the GDS connects to")
    args = parser.parse_args()

    master, slave = os.openpty()
    tty.setraw(slave)
    if os.path.lexists(args.fsw_link):
        os.unlink(args.fsw_link)
    os.symlink(os.ttyname(slave), args.fsw_link)
    # Keep the slave open so the pty survives the FSW closing and reopening it

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("127.0.0.1", args.port))
    server.listen(1)
    print(f"FSW side: {args.fsw_link} -> {os.ttyname(slave)}; GDS side: tcp 127.0.0.1:{args.port}", flush=True)

    def stop(*_):
        if os.path.islink(args.fsw_link):
            os.unlink(args.fsw_link)
        sys.exit(0)

    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)

    client = None
    while True:
        watched = [master, server] + ([client] if client else [])
        readable, _, _ = select.select(watched, [], [])
        if server in readable:
            if client:
                client.close()
            client, _ = server.accept()
        if master in readable:
            try:
                data = os.read(master, 4096)
            except OSError:
                data = b""
            if data and client:
                try:
                    client.sendall(data)
                except OSError:
                    client.close()
                    client = None
        if client and client in readable:
            data = client.recv(4096)
            if data:
                os.write(master, data)
            else:
                client.close()
                client = None


if __name__ == "__main__":
    main()
