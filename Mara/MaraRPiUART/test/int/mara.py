"""Helpers for the MARA integration tests.

The tests talk to a running GDS through the F´ GDS pytest plugin (`fprime_test_api`). These
helpers add the deployment's names, command checks that match how this deployment responds,
and access to the Pi over ssh (GPIO lines, the fake drive, files, sequences).
"""

import json
import re
import shlex
import struct
import subprocess
import time
from pathlib import Path

CMD_DISP = "CdhCore.cmdDisp"
ORCH = "Mara.orchestrator"
MOTOR = "Mara.platformMotor"
UART = "Mara.motorUart"
CAMERA = "Mara.opticalCamera"
GPIO = "Mara.gpioWatcher"
SEQ = "Mara.cmdSeq"

TEST_COMMANDS = [
    ("testDrillON", []),
    ("testDrillOFF", []),
    ("testPlatformMoveTo", [100]),
    ("testPlatformStop", []),
    ("testOpticalCameraON", []),
    ("testOpticalCameraOFF", []),
    ("testThermalCameraON", []),
    ("testThermalCameraOFF", []),
]

# CiA 402 objects and controlwords (see PlatformMotor)
CONTROLWORD = 0x6040
MODE_OF_OPERATION = 0x6060
TARGET_POSITION = 0x607A
PROFILE_VELOCITY = 0x6081
PROFILE_ACCELERATION = 0x6083

REPO_ROOT = Path(__file__).resolve().parents[4]
SEQ_DIR = REPO_ROOT / "Mara" / "MaraRPiUART" / "seq"
TOPOLOGY_DEFS = REPO_ROOT / "Mara" / "MaraRPiUART" / "Top" / "MaraRPiUARTTopologyDefs.hpp"


def motor_uart_device():
    """The platform motor UART path compiled into the FSW (MotorUartDevice)."""
    match = re.search(r'MotorUartDevice\s*\{\s*"([^"]+)"', TOPOLOGY_DEFS.read_text())
    assert match, f"MotorUartDevice not found in {TOPOLOGY_DEFS}"
    return match.group(1)


class Mara:
    """Commands and checks for the MARA deployment on top of the F´ test API."""

    def __init__(self, api):
        self.api = api

    # ------------------------------------------------------------------
    # Commands
    # ------------------------------------------------------------------

    def cmd(self, name, args=None, timeout=5):
        """Send a command and assert it completed OK."""
        return self.api.send_and_assert_command(name, args or [], timeout=timeout, commander=CMD_DISP)

    def orch(self, name, args=None, timeout=5):
        return self.cmd(f"{ORCH}.{name}", args, timeout)

    def cmd_rejected(self, name, args=None, timeout=5):
        """Send a command and assert it was answered EXECUTION_ERROR."""
        opcode = self.api.translate_command_name(name)
        error = self.api.get_event_pred(f"{CMD_DISP}.OpCodeError", [opcode, "EXECUTION_ERROR"])
        return self.api.send_and_assert_event(name, args or [], [error], timeout=timeout)

    def prm_set(self, component, param, value):
        self.cmd(f"{component}.{param}_PRM_SET", [value])

    def set_motor_params(self, min_pos=0, max_pos=1000, advance=0, retract=0, accel=0, node=1):
        for param, value in [
            ("MIN_POSITION", min_pos),
            ("MAX_POSITION", max_pos),
            ("ADVANCE_VELOCITY", advance),
            ("RETRACT_VELOCITY", retract),
            ("ACCELERATION", accel),
            ("NODE_ID", node),
        ]:
            self.prm_set(MOTOR, param, value)

    def set_timeline(self, spin_up, advance, retract, drill_position):
        for param, value in [
            ("SPIN_UP_SECONDS", spin_up),
            ("ADVANCE_SECONDS", advance),
            ("RETRACT_SECONDS", retract),
            ("DRILL_POSITION", drill_position),
        ]:
            self.prm_set(ORCH, param, value)

    def leave_test_mode(self):
        """Best-effort exit from TEST (ignored by the FSW if not in TEST)."""
        try:
            self.api.send_command(f"{ORCH}.exitTestMode")
            time.sleep(0.5)
        except Exception:  # noqa: BLE001 - teardown must not raise
            pass

    # ------------------------------------------------------------------
    # Events and telemetry
    # ------------------------------------------------------------------

    def await_event(self, name, args=None, timeout=5):
        """Assert an event is in this test's history, waiting up to `timeout` s for it.

        Searches the whole test history (cleared at the start of each test), because events
        caused by a command usually arrive before the command's completion is seen.
        """
        return self.api.assert_event(name, args=args, timeout=timeout)

    def event_count(self, name, window=0.0):
        """Number of events with this name in this test's history, after waiting `window` seconds."""
        if window:
            time.sleep(window)
        return len(self.api.await_event_count(lambda n: True, name, start=0, timeout=0))

    def warnings_hi(self):
        """WARNING_HI events in this test's history."""
        from fprime_gds.common.utils.event_severity import EventSeverity

        pred = self.api.get_event_pred(severity=EventSeverity.WARNING_HI)
        return [e for e in self.api.get_event_test_history().retrieve() if pred(e)]

    def latest_telemetry(self, channel, timeout=5):
        found = self.api.await_telemetry(channel, timeout=timeout)
        assert found is not None, f"no {channel} telemetry within {timeout} s"
        return found.get_val()

    # ------------------------------------------------------------------
    # Sequences
    # ------------------------------------------------------------------

    def compile_sequence(self, name, out_dir):
        """Compile Mara/MaraRPiUART/seq/<name>.seq against the dictionary; return the .bin path."""
        out = Path(out_dir) / f"{name}.bin"
        result = subprocess.run(
            [
                "fprime-seqgen",
                "--dictionary",
                str(self.api.dictionaries.dictionary_path),
                str(SEQ_DIR / f"{name}.seq"),
                str(out),
            ],
            capture_output=True,
            text=True,
        )
        assert result.returncode == 0, f"fprime-seqgen failed:\n{result.stdout}\n{result.stderr}"
        return out


class PiShell:
    """Runs commands on the Pi over ssh."""

    def __init__(self, host):
        self.host = host

    def run(self, command, check=True, timeout=30):
        result = subprocess.run(
            ["ssh", "-o", "BatchMode=yes", self.host, command], capture_output=True, text=True, timeout=timeout
        )
        if check:
            assert result.returncode == 0, f"ssh {self.host} '{command}' failed:\n{result.stderr}"
        return result

    def popen(self, command):
        return subprocess.Popen(
            ["ssh", "-o", "BatchMode=yes", self.host, command], stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )

    def copy_to(self, local, remote):
        result = subprocess.run(["scp", "-q", "-o", "BatchMode=yes", str(local), f"{self.host}:{remote}"])
        assert result.returncode == 0, f"scp {local} to {self.host}:{remote} failed"

    def read_bytes(self, remote):
        result = subprocess.run(["ssh", "-o", "BatchMode=yes", self.host, f"cat {remote}"], capture_output=True)
        return result.stdout if result.returncode == 0 else None


class GpioDriver:
    """Drives spare Pi output pins that are wired to the LO/SOE/EODS inputs, with libgpiod's gpioset."""

    def __init__(self, pi, chip, pins):
        self.pi = pi
        self.chip = chip
        self.pins = pins  # {"LO": 5, "SOE": 6, "EODS": 13}
        version = pi.run("gpioset --version", check=False).stdout
        self.v2 = re.search(r"v2\.", version) is not None

    def pulse(self, line, seconds):
        """Hold a line HIGH for `seconds`, then release it (blocks)."""
        pin = self.pins[line]
        if self.v2:
            command = f"timeout {seconds} gpioset -c {self.chip} {pin}=1"
        else:
            command = f"timeout {seconds} gpioset --mode=signal {self.chip} {pin}=1"
        self.pi.run(command, check=False, timeout=seconds + 10)  # timeout's exit code 124 is expected


class FakeDrive:
    """Controls fake_drive.py on the Pi: a pty at the motor UART path that records Waveshare frames."""

    SCRIPT = "/tmp/mara_fake_drive.py"
    LOG = "/tmp/mara_fake_drive.jsonl"

    def __init__(self, pi):
        self.pi = pi
        self.device = motor_uart_device()
        pi.copy_to(Path(__file__).parent / "fake_drive.py", self.SCRIPT)

    def start(self):
        self.stop()
        self.pi.run(
            f"sudo nohup python3 {self.SCRIPT} --link {shlex.quote(self.device)} --log {self.LOG} "
            f">/tmp/mara_fake_drive.out 2>&1 &"
        )
        time.sleep(0.5)

    def stop(self):
        self.pi.run(f"sudo pkill -f {self.SCRIPT}", check=False)
        time.sleep(0.5)

    def clear(self):
        self.pi.run(f"sudo truncate -s 0 {self.LOG}", check=False)

    def frames(self):
        """Decoded SDO downloads received so far, oldest first."""
        out = self.pi.run(f"cat {self.LOG}", check=False).stdout
        return [json.loads(line) for line in out.splitlines() if line.strip()]

    def sdos(self):
        """(index, value) of each SDO download, oldest first."""
        return [(f["index"], f["value"]) for f in self.frames() if f.get("index") is not None]

    def wait_for(self, count, timeout=5):
        deadline = time.time() + timeout
        while time.time() < deadline:
            sdos = self.sdos()
            if len(sdos) >= count:
                return sdos
            time.sleep(0.2)
        return self.sdos()


def read_prmdb(raw, dictionary_params):
    """Effective parameter values: dictionary defaults overridden by a PrmDb.dat image.

    PrmDb record: 0xA5, U32 record size (BE), U32 parameter ID (BE), value (BE).
    """
    by_id = {p["id"]: p for p in dictionary_params}
    values = {p["name"]: p.get("default") for p in dictionary_params}
    offset = 0
    while raw and offset + 9 <= len(raw):
        assert raw[offset] == 0xA5, f"bad PrmDb delimiter at byte {offset}"
        size, prm_id = struct.unpack_from(">II", raw, offset + 1)
        value = raw[offset + 9 : offset + 5 + size]
        param = by_id.get(prm_id)
        if param is not None:
            kind = param["type"]["name"]
            fmt = {"I32": ">i", "U32": ">I", "U8": ">B", "I16": ">h", "U16": ">H"}.get(kind)
            if fmt is not None:
                values[param["name"]] = struct.unpack(fmt, value)[0]
        offset += 5 + size
    return values
