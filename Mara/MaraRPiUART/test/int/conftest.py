"""pytest configuration for the MARA integration tests.

Suites, selected by marker and by what the run is pointed at (see README.md):

  software   any running FSW without hardware: laptop (native build) or dev Pi
  devpi      FSW on the dev Pi with nothing attached
  fakedrive  dev Pi with fake_drive.py standing in for the motor drive
  gpio       dev Pi with spare outputs wired to LO/SOE/EODS: flight-timeline rehearsal
  hardware   flight Pi with the real hardware; operator present (run with -s)

Every test ends with exitTestMode, so the hardware is left in a known state even when a test fails.
"""

import json
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent))

from mara import FakeDrive, GpioDriver, Mara, PiShell  # noqa: E402

TARGETS = {
    "software": {"laptop", "devpi"},
    "devpi": {"devpi"},
    "fakedrive": {"devpi"},
    "gpio": {"devpi"},
    "hardware": {"flightpi"},
}


def pytest_addoption(parser):
    group = parser.getgroup("mara", "MARA integration tests")
    group.addoption(
        "--mara-target",
        choices=["laptop", "devpi", "flightpi"],
        default="laptop",
        help="where the FSW under test runs [default: laptop]",
    )
    group.addoption("--mara-pi", default=None, help="ssh destination of the Pi running the FSW, e.g. pi@mara-dev")
    group.addoption("--mara-fsw-dir", default="~", help="directory on the Pi holding the MARA binary [default: ~]")
    group.addoption(
        "--mara-fsw-args", default="-d /dev/serial0 -b 115200", help="arguments the FSW is started with on the Pi"
    )
    group.addoption("--mara-fake-drive", action="store_true", help="run fake_drive.py on the Pi (needs sudo)")
    group.addoption("--mara-gpio-chip", default="gpiochip4", help="gpiochip of the jumpered outputs")
    group.addoption(
        "--mara-gpio-drive",
        default=None,
        help="BCM output pins wired to the LO,SOE,EODS inputs, e.g. 5,6,13",
    )
    group.addoption("--mara-test-position", type=int, default=None, help="small safe platform target (counts)")
    group.addoption(
        "--mara-camera-dirs",
        default="/mnt/sd1/camera,/mnt/sd2/camera",
        help="primary,backup camera output dirs on the flight Pi",
    )


def pytest_configure(config):
    for marker, description in [
        ("software", "any running FSW, no hardware needed"),
        ("devpi", "FSW on the dev Pi, nothing attached"),
        ("fakedrive", "dev Pi with the fake drive"),
        ("gpio", "dev Pi with LO/SOE/EODS jumpered to spare outputs"),
        ("hardware", "flight Pi with the real hardware, operator present"),
    ]:
        config.addinivalue_line("markers", f"{marker}: {description}")


def pytest_collection_modifyitems(config, items):
    target = config.getoption("--mara-target")
    has_pi = config.getoption("--mara-pi") is not None
    for item in items:
        suites = [m.name for m in item.iter_markers() if m.name in TARGETS]
        for suite in suites:
            reason = None
            if target not in TARGETS[suite]:
                reason = f"{suite} tests need --mara-target {'/'.join(sorted(TARGETS[suite]))}"
            elif suite != "software" and not has_pi:
                reason = f"{suite} tests need --mara-pi"
            elif suite in ("fakedrive", "gpio") and not config.getoption("--mara-fake-drive"):
                reason = f"{suite} tests need --mara-fake-drive"
            elif suite == "gpio" and config.getoption("--mara-gpio-drive") is None:
                reason = "gpio tests need --mara-gpio-drive"
            elif suite == "hardware" and config.getoption("capture") != "no":
                reason = "hardware tests ask the operator questions: run with -s"
            if reason:
                item.add_marker(pytest.mark.skip(reason=reason))


@pytest.fixture
def mara(fprime_test_api):
    """MARA helpers; always leaves TEST mode afterwards (drill and camera off, platform to 0)."""
    helper = Mara(fprime_test_api)
    yield helper
    helper.leave_test_mode()


@pytest.fixture(scope="session")
def pi(request):
    host = request.config.getoption("--mara-pi")
    if host is None:
        pytest.skip("needs --mara-pi")
    return PiShell(host)


@pytest.fixture(scope="session")
def dictionary(fprime_test_api_session):
    return json.loads(Path(fprime_test_api_session.dictionaries.dictionary_path).read_text())


@pytest.fixture
def restart_fsw(request, pi, fprime_test_api):
    """Restart the FSW on the Pi (the only way back to IDLE after LO). Returns a callable."""

    def restart():
        fsw_dir = request.config.getoption("--mara-fsw-dir")
        fsw_args = request.config.getoption("--mara-fsw-args")
        pi.run("pkill -INT -x MARA; sleep 2; pkill -KILL -x MARA", check=False)
        pi.run(f"cd {fsw_dir} && nohup ./MARA {fsw_args} > /tmp/mara_fsw.log 2>&1 &")
        # Streaming again
        found = fprime_test_api.await_telemetry_count(3, timeout=20)
        assert len(found) >= 3, "FSW did not come back after restart"
        fprime_test_api.clear_histories()

    return restart


@pytest.fixture
def fake_drive(request, pi, fprime_test_api):
    """The fake drive, running and connected. Stopped afterwards."""
    if not request.config.getoption("--mara-fake-drive"):
        pytest.skip("needs --mara-fake-drive")
    drive = FakeDrive(pi)
    drive.start()
    drive.clear()
    yield drive
    drive.stop()


@pytest.fixture
def gpio(request, pi):
    pins = request.config.getoption("--mara-gpio-drive")
    if pins is None:
        pytest.skip("needs --mara-gpio-drive")
    lo, soe, eods = (int(p) for p in pins.split(","))
    return GpioDriver(pi, request.config.getoption("--mara-gpio-chip"), {"LO": lo, "SOE": soe, "EODS": eods})


class Operator:
    """Questions for the person at the bench (hardware tests run with -s)."""

    def confirm(self, question):
        answer = input(f"\n>>> {question} [y/n] ").strip().lower()
        assert answer.startswith("y"), f"operator answered no: {question}"

    def do(self, instruction):
        input(f"\n>>> {instruction} (press Enter when done) ")


@pytest.fixture
def operator():
    return Operator()


@pytest.fixture
def test_position(request):
    position = request.config.getoption("--mara-test-position")
    if position is None:
        pytest.skip("needs --mara-test-position (a small, safe platform target in counts)")
    return position
