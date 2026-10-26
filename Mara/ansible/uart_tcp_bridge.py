import socket
import argparse
import sys
import logging
import threading
import serial
import time
import selectors

class Bridge:
    def __init__(self, serial_port: str, baudrate: int, host: str, port: int, serial_timeout: float = 0.1):
        self.serial_port_name = serial_port
        self.baudrate = baudrate
        self.host = host
        self.port = port
        self.serial_timeout = serial_timeout

        self.stop_event = threading.Event()
        self.clients: Set[socket.socket] = set()
        self.clients_lock = threading.Lock()
        self.serial_lock = threading.Lock()
        self.server_socket: Optional[socket.socket] = None
        self.ser: Optional[serial.Serial] = None

    def open_serial(self) -> None:
        self.ser = serial.Serial(
            port=self.serial_port_name,
            baudrate=self.baudrate,
            timeout=self.serial_timeout,
            write_timeout=1,
        )
        logging.info("Opened serial port %s at %d baud", self.serial_port_name, self.baudrate)

    def start_server(self) -> None:
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((self.host, self.port))
        srv.listen()
        srv.setblocking(False)
        self.server_socket = srv
        logging.info("Listening on TCP %s:%d", self.host, self.port)

    def add_client(self, client: socket.socket, addr) -> None:
        client.setblocking(False)
        client.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        with self.clients_lock:
            self.clients.add(client)
        logging.info("Client connected: %s:%s", addr[0], addr[1])

    def remove_client(self, client: socket.socket) -> None:
        with self.clients_lock:
            if client in self.clients:
                self.clients.remove(client)
        try:
            peer = client.getpeername()
            logging.info("Client disconnected: %s:%s", peer[0], peer[1])
        except OSError:
            logging.info("Client disconnected")
        try:
            client.close()
        except OSError:
            pass

    def broadcast_from_serial(self, data: bytes) -> None:
        dead = []
        with self.clients_lock:
            clients = list(self.clients)
        for client in clients:
            try:
                client.sendall(data)
            except OSError:
                dead.append(client)
        for client in dead:
            self.remove_client(client)

    def serial_reader_loop(self) -> None:
        assert self.ser is not None
        while not self.stop_event.is_set():
            try:
                data = self.ser.read(4096)
                if data:
                    logging.debug("UART -> TCP %r", data)
                    self.broadcast_from_serial(data)
            except serial.SerialException as exc:
                logging.error("Serial read failed: %s", exc)
                self.stop_event.set()
                break

    def client_reader_loop(self) -> None:
        assert self.server_socket is not None
        sel = selectors.DefaultSelector()
        sel.register(self.server_socket, selectors.EVENT_READ, data="server")

        while not self.stop_event.is_set():
            with self.clients_lock:
                current_clients = list(self.clients)
            registered = {key.fileobj for key in sel.get_map().values()}
            for client in current_clients:
                if client not in registered:
                    try:
                        sel.register(client, selectors.EVENT_READ, data="client")
                    except Exception:
                        self.remove_client(client)
            for fileobj in list(registered):
                if fileobj is self.server_socket:
                    continue
                if fileobj not in current_clients:
                    try:
                        sel.unregister(fileobj)
                    except Exception:
                        pass

            events = sel.select(timeout=0.2)
            for key, _ in events:
                if key.data == "server":
                    try:
                        client, addr = self.server_socket.accept()
                        self.add_client(client, addr)
                    except OSError:
                        continue
                else:
                    client = key.fileobj
                    try:
                        data = client.recv(4096)
                    except OSError:
                        self.remove_client(client)
                        try:
                            sel.unregister(client)
                        except Exception:
                            pass
                        continue

                    if not data:
                        self.remove_client(client)
                        try:
                            sel.unregister(client)
                        except Exception:
                            pass
                        continue

                    logging.debug("TCP -> UART %r", data)
                    try:
                        with self.serial_lock:
                            assert self.ser is not None
                            self.ser.write(data)
                            self.ser.flush()
                    except (serial.SerialException, OSError) as exc:
                        logging.error("Serial write failed: %s", exc)
                        self.stop_event.set()
                        break

        sel.close()

    def shutdown(self) -> None:
        self.stop_event.set()
        with self.clients_lock:
            clients = list(self.clients)
            self.clients.clear()
        for client in clients:
            try:
                client.close()
            except OSError:
                pass
        if self.server_socket is not None:
            try:
                self.server_socket.close()
            except OSError:
                pass
        if self.ser is not None:
            try:
                self.ser.close()
            except Exception:
                pass

    def run(self) -> int:
        self.open_serial()
        self.start_server()

        t_serial = threading.Thread(target=self.serial_reader_loop, name="serial-reader", daemon=True)
        t_tcp = threading.Thread(target=self.client_reader_loop, name="tcp-server", daemon=True)
        t_serial.start()
        t_tcp.start()

        try:
            while not self.stop_event.is_set():
                time.sleep(0.2)
        except KeyboardInterrupt:
            logging.info("Stopping bridge")
            self.stop_event.set()
        finally:
            self.shutdown()
            t_serial.join(timeout=1)
            t_tcp.join(timeout=1)
        return 0


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Bridge one UART to a local TCP server for GDS testing")
    p.add_argument("--serial-port", required=True, help="Serial device, e.g. /dev/tty.usbserial-0001 or /dev/ttyUSB0")
    p.add_argument("--baudrate", type=int, default=115200, help="UART baudrate (default: 115200)")
    p.add_argument("--host", default="127.0.0.1", help="TCP listen address (default: 127.0.0.1)")
    p.add_argument("--port", type=int, default=50000, help="TCP listen port (default: 50000)")
    p.add_argument("--verbose", action="store_true", help="Enable debug logging")
    return p.parse_args()


def main() -> int:
    args = parse_args()
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
    )

    bridge = Bridge(
        serial_port=args.serial_port,
        baudrate=args.baudrate,
        host=args.host,
        port=args.port,
    )
    return bridge.run()


if __name__ == "__main__":
    sys.exit(main())
