"""devpi: the real Linux FSW on the dev Pi with nothing attached.

The flight software must start and keep running without the motor adapter or GPIO wiring,
warn once, and fail platform commands cleanly (no resets, no event floods).
"""

import time

import pytest

from mara import GPIO, MOTOR, ORCH, UART

pytestmark = pytest.mark.devpi


def test_adapter_missing_warns_once(mara, restart_fsw):
    """After a fresh start without the adapter: exactly one AdapterNotConnected, even over time."""
    restart_fsw()
    mara.await_event(f"{UART}.AdapterNotConnected", timeout=5)
    time.sleep(10)
    mara.api.assert_event_count(1, f"{UART}.AdapterNotConnected")
    mara.api.assert_event_count(0, f"{UART}.AdapterConnected")


def test_platform_commands_fail_cleanly(mara):
    """Each platform command logs exactly one SendFailed and does nothing else; the FSW stays up."""
    mara.set_motor_params(min_pos=0, max_pos=1000)
    mara.orch("enterTestMode")
    mara.await_event(f"{MOTOR}.SendFailed", timeout=3)  # the enable
    mara.api.assert_event_count(0, f"{MOTOR}.Enabled")

    mara.api.clear_histories()
    mara.orch("testPlatformMoveTo", [100])
    mara.orch("testPlatformStop")
    time.sleep(2)
    mara.api.assert_event_count(2, f"{MOTOR}.SendFailed")
    mara.api.assert_event_count(0, f"{MOTOR}.MovingTo")
    mara.api.assert_event_count(0, f"{MOTOR}.Stopped")

    # No flood, and still streaming
    assert len(mara.api.get_event_test_history().retrieve()) < 50
    found = mara.api.await_telemetry_count(3, f"{ORCH}.PhaseSeconds", timeout=10)
    assert len(found) >= 3


def test_gpio_lines_quiet(mara):
    """With nothing driving LO/SOE/EODS, no signal fires."""
    time.sleep(5)
    for line in ("LO", "SOE", "EODS"):
        mara.api.assert_event_count(0, f"{GPIO}.{line}Detected")
    mara.api.assert_event_count(0, f"{ORCH}.EnterFlight")
