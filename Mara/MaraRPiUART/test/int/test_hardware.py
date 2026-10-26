"""hardware: the flight Pi with the real drive, platform, camera and accelerometer.

An operator must be at the bench: motion steps ask for confirmation, and answering "n" fails
the test, after which the teardown leaves TEST mode (drill and camera off, platform to 0).
Run with -s so the questions reach the terminal. Nothing here changes the Pi's system state
or saves parameters.

PlatformMotor does not read the drive's replies yet, so whether the platform actually moved
is confirmed by the operator, not by telemetry.
"""

import math
import time

import pytest

from mara import CAMERA, MOTOR, ORCH, UART, motor_uart_device, read_prmdb

pytestmark = pytest.mark.hardware


@pytest.fixture
def in_test_mode(mara):
    """TEST mode with the drive enabled; fails if the platform limits were never set."""
    mara.orch("enterTestMode")
    mara.await_event(f"{MOTOR}.Enabled", timeout=3)
    time.sleep(1)
    assert mara.event_count(f"{MOTOR}.LimitsUnset") == 0, "MIN/MAX_POSITION are both 0: set them first"
    mara.api.assert_event_count(0, f"{MOTOR}.SendFailed")
    return mara


def test_motor_link(in_test_mode):
    """The enable went out over a working link: no send failures and bytes on the wire."""
    mara = in_test_mode
    sent = mara.latest_telemetry(f"{UART}.BytesSent")
    assert sent >= 5 * 13, "the enable is at least 5 frames of 13 bytes"


def test_platform_moves_and_returns(in_test_mode, operator, test_position):
    mara = in_test_mode
    for attempt in range(1, 4):
        mara.orch("testPlatformMoveTo", [test_position])
        mara.await_event(f"{MOTOR}.MovingTo", [test_position])
        operator.confirm(f"[{attempt}/3] Did the platform move towards the sample by the expected amount?")
        mara.orch("testPlatformMoveTo", [0])
        operator.confirm(f"[{attempt}/3] Is the platform back exactly at its starting mark?")
    mara.api.assert_event_count(0, f"{MOTOR}.SendFailed")


def test_clamped_at_limit(in_test_mode, operator):
    mara = in_test_mode
    mara.orch("testPlatformMoveTo", [2**31 - 1])
    clamped = mara.await_event(f"{MOTOR}.PositionClamped")
    limit = clamped.get_args()[1].val
    operator.confirm(f"Did the platform stop at the upper limit (MAX_POSITION = {limit} counts)?")
    mara.orch("testPlatformMoveTo", [0])
    operator.confirm("Is the platform back at its starting mark?")


def test_adapter_hot_plug(in_test_mode, operator):
    mara = in_test_mode
    operator.do("Unplug the USB-CAN adapter")
    mara.await_event(f"{UART}.AdapterDisconnected", timeout=5)
    mara.orch("testPlatformStop")
    mara.await_event(f"{MOTOR}.SendFailed", timeout=3)

    operator.do("Plug the USB-CAN adapter back in")
    mara.await_event(f"{UART}.AdapterConnected", timeout=10)
    mara.api.clear_histories()
    mara.orch("testPlatformStop")
    mara.await_event(f"{MOTOR}.Stopped", timeout=3)
    mara.api.assert_event_count(0, f"{MOTOR}.SendFailed")


def test_camera_records_to_both_cards(mara, pi, request):
    mara.orch("enterTestMode")
    mara.orch("testOpticalCameraON")
    started = mara.await_event(f"{CAMERA}.RecordingStarted", timeout=5)
    segment = started.get_args()[0].val
    time.sleep(10)
    mara.orch("testOpticalCameraOFF")
    mara.await_event(f"{CAMERA}.RecordingStopped", timeout=10)
    mara.api.assert_event_count(0, f"{CAMERA}.RecorderStalled")
    mara.api.assert_event_count(0, f"{CAMERA}.RecorderExitedEarly")

    name = f"cam_{segment:03d}.mkv"
    sizes = []
    for directory in request.config.getoption("--mara-camera-dirs").split(","):
        result = pi.run(f"stat -c %s {directory}/{name}", check=False)
        assert result.returncode == 0, f"{directory}/{name} missing"
        sizes.append(int(result.stdout.strip()))
    assert all(size > 100_000 for size in sizes), f"recordings too small: {sizes}"
    assert abs(sizes[0] - sizes[1]) <= 0.01 * max(sizes), f"cards differ: {sizes}"


def test_accelerometer_at_rest(mara):
    """At rest the accelerometer reads about 1 g, updating at 1 Hz."""
    found = mara.api.await_telemetry_count(3, "Mara.adxl345Manager.accelZ", timeout=5)
    assert len(found) >= 3, "accelerometer telemetry not updating"
    x = mara.latest_telemetry("Mara.adxl345Manager.accelX")
    y = mara.latest_telemetry("Mara.adxl345Manager.accelY")
    z = mara.latest_telemetry("Mara.adxl345Manager.accelZ")
    magnitude = math.sqrt(x * x + y * y + z * z)
    assert abs(magnitude - 1.0) <= 0.15, f"|a| = {magnitude:.3f} g at rest"


def test_preflight_checklist(mara, pi, dictionary, request):
    """Run last before integration. Reads state; moves nothing."""
    # Not in TEST (ESA: LO is ignored in TEST). A test command must be rejected.
    mara.cmd_rejected(f"{ORCH}.testPlatformStop")

    # Flight parameters as the FSW will load them at boot: saved PrmDb over dictionary defaults
    fsw_dir = request.config.getoption("--mara-fsw-dir")
    raw = pi.read_bytes(f"{fsw_dir}/PrmDb.dat")
    params = read_prmdb(raw, dictionary["parameters"])
    drill = params["Mara.orchestrator.DRILL_POSITION"]
    low = params["Mara.platformMotor.MIN_POSITION"]
    high = params["Mara.platformMotor.MAX_POSITION"]
    assert drill != 0, "DRILL_POSITION is 0: the platform would not move"
    assert (low, high) != (0, 0), "MIN/MAX_POSITION are both 0"
    assert low <= 0 <= high, f"0 must be inside [{low}, {high}], or the retract is clamped short of the bottom"
    assert low <= drill <= high, f"DRILL_POSITION {drill} is outside [{low}, {high}] and would be clamped"
    for name in ("SPIN_UP_SECONDS", "ADVANCE_SECONDS", "RETRACT_SECONDS"):
        assert params[f"Mara.orchestrator.{name}"] > 0, f"{name} is 0"

    # The adapter is present at the path the FSW opens
    assert pi.run(f"test -e {motor_uart_device()}", check=False).returncode == 0, "USB-CAN adapter not present"

    # Space for the recordings on both cards (at least 4 GB each)
    for directory in request.config.getoption("--mara-camera-dirs").split(","):
        free_kb = int(pi.run(f"df -Pk {directory} | tail -1 | awk '{{print $4}}'").stdout.strip())
        assert free_kb > 4 * 1024 * 1024, f"{directory}: only {free_kb // 1024} MB free"

    # Nothing is warning right now
    time.sleep(10)
    assert mara.warnings_hi() == [], f"WARNING_HI events: {mara.warnings_hi()}"
