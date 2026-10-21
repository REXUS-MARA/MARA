"""stability: soak tests for leaks and instability in the running FSW.

The unit tests run under AddressSanitizer/LeakSanitizer/UBSan/ThreadSanitizer; these tests
look at the real deployment over time instead. They repeat a realistic command workload and
watch the FSW process itself (resident memory, open file descriptors), health pings and
liveness. Memory must stop growing after warm-up, and nothing may leak per cycle.
"""

import random
import re
import subprocess
import sys
import time

import pytest

from mara import ORCH, TEST_COMMANDS, UART

pytestmark = pytest.mark.stability

RSS_GROWTH_LIMIT_KB = 2048  # after warm-up, over the whole soak
FD_GROWTH_LIMIT = 2


class FswProcess:
    """Resident memory and open file descriptors of the FSW process, locally or on the Pi."""

    def __init__(self, request):
        host = request.config.getoption("--mara-pi")
        self.target = request.config.getoption("--mara-target")
        self.pi = None
        if self.target != "laptop" and host is not None:
            from mara import PiShell

            self.pi = PiShell(host)
        # The FSW executable's name: ~/MARA on the Pi, the build output locally. Matched on the
        # executable's basename, so neither the GDS (whose command line contains the dictionary
        # path) nor the shell running the query can match.
        self.exe = "MARA" if self.pi else "Mara_MaraRPiUART"

    def _run(self, command):
        if self.pi:
            return self.pi.run(command, check=False).stdout
        return subprocess.run(command, shell=True, capture_output=True, text=True).stdout

    def pid(self):
        matches = []
        for line in self._run("ps -axo pid=,comm=").splitlines():
            parts = line.split(None, 1)
            if len(parts) == 2 and parts[1].strip().split("/")[-1] == self.exe:
                matches.append(int(parts[0]))
        assert len(matches) == 1, f"expected exactly one {self.exe} process, found {matches}"
        return matches[0]

    def rss_kb(self):
        return int(self._run(f"ps -o rss= -p {self.pid()}").strip())

    def local_macos(self):
        return self.pi is None and sys.platform == "darwin"

    def leaked_blocks(self):
        """Leaked heap blocks reported by macOS `leaks` for a local FSW; None where unavailable."""
        if self.pi or sys.platform != "darwin":
            return None
        out = self._run(f"leaks {self.pid()} 2>&1")
        match = re.search(r"(\d+) leaks? for", out)
        return int(match.group(1)) if match else None

    def open_fds(self):
        """Open file descriptors (Linux only, via /proc); None elsewhere."""
        out = self._run(f"ls /proc/{self.pid()}/fd 2>/dev/null | wc -l").strip()
        return int(out) if out and out != "0" else None


@pytest.fixture
def fsw(request):
    return FswProcess(request)


def _workload_cycle(mara, rng):
    """One round of what the ground does: TEST mode, a few test commands, rejected commands."""
    mara.orch("enterTestMode")
    for command, args in rng.sample(TEST_COMMANDS, 4):
        if command in ("testOpticalCameraON", "testOpticalCameraOFF"):
            continue  # starting ffmpeg every few seconds is not a realistic load
        mara.orch(command, [rng.randint(-1000, 1000)] if args else [])
    mara.orch("exitTestMode")
    mara.cmd_rejected(f"{ORCH}.testPlatformStop")
    mara.cmd_rejected(f"{ORCH}.exitTestMode")


def test_soak(mara, fsw, request):
    """Repeat the command workload for --mara-soak-minutes; memory must plateau, health stays clean.

    Resident memory grows at first (thread stacks and allocator caches are touched for the first
    time), so the check is that the second half of the soak is flat, not that RSS never grows.
    On macOS the `leaks` tool also checks the live heap for leaked blocks.
    """
    minutes = request.config.getoption("--mara-soak-minutes")
    rng = random.Random(1234)
    pid = fsw.pid()
    baseline_fds = fsw.open_fds()

    deadline = time.time() + minutes * 60
    samples = []
    while time.time() < deadline:
        _workload_cycle(mara, rng)
        samples.append(fsw.rss_kb())
        if len(samples) % 20 == 0:
            mara.api.clear_histories()  # keep the test-side history small

    assert fsw.pid() == pid, "the FSW restarted during the soak"
    second_half = samples[len(samples) // 2 :]
    growth = max(second_half) - min(second_half)
    print(f"\nsoak: {len(samples)} cycles in {minutes} min, RSS samples (kB): {samples}")
    assert len(samples) >= 10, "soak too short to judge; use --mara-soak-minutes >= 3"
    if fsw.local_macos():
        # Known F´ 4.1.1 bug, macOS only: Os/Darwin/Cpu.cpp never vm_deallocate()s the array
        # host_processor_info() returns, so SystemResources leaks ~110 kB/s on a Mac. The Pi
        # (Linux) reads /proc/stat instead. On a Mac, rely on `leaks` below for the heap.
        print(f"macOS: RSS plateau check skipped (F´ Os::Darwin::Cpu leak); second-half growth +{growth} kB")
    else:
        assert growth < RSS_GROWTH_LIMIT_KB, f"resident memory still growing in the second half: +{growth} kB"
    if baseline_fds is not None:
        assert fsw.open_fds() - baseline_fds <= FD_GROWTH_LIMIT, "file descriptors leak"
    leaked = fsw.leaked_blocks()
    if leaked is not None:
        assert leaked == 0, f"leaks reports {leaked} leaked blocks in the FSW"

    # Still healthy and responsive
    mara.api.assert_event_count(0, "CdhCore.health.HLTH_PING_WARN")
    found = mara.api.await_telemetry_count(3, f"{ORCH}.PhaseSeconds", timeout=10)
    assert len(found) >= 3


def test_command_burst(mara, fsw):
    """A burst of commands faster than the ground would send: the FSW must neither crash nor stall.

    Commands beyond the Orchestrator's queue may be dropped (no response) by design; what
    matters is that the FSW stays alive and answers normally afterwards.
    """
    pid = fsw.pid()
    for _ in range(200):
        mara.api.send_command(f"{ORCH}.testPlatformStop")
    time.sleep(5)
    assert fsw.pid() == pid, "the FSW restarted"
    mara.api.clear_histories()
    mara.orch("enterTestMode")
    mara.orch("exitTestMode")
    mara.api.assert_event_count(0, "CdhCore.health.HLTH_PING_WARN")


@pytest.mark.fakedrive
def test_reconnect_soak(mara, fsw, fake_drive):
    """Many adapter unplug/replug cycles leak neither file descriptors nor memory."""
    mara.await_event(f"{UART}.AdapterConnected", timeout=5)
    baseline_fds = fsw.open_fds()
    baseline_rss = fsw.rss_kb()

    for cycle in range(1, 21):
        fake_drive.stop()
        mara.api.await_event(f"{UART}.AdapterDisconnected", timeout=5)
        fake_drive.start()
        mara.api.await_event(f"{UART}.AdapterConnected", timeout=5)
        if cycle % 5 == 0:
            mara.api.clear_histories()

    assert fsw.open_fds() - baseline_fds <= FD_GROWTH_LIMIT, "file descriptors leak across reconnects"
    assert fsw.rss_kb() - baseline_rss < RSS_GROWTH_LIMIT_KB, "memory grows across reconnects"
