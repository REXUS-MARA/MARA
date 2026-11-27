"""fakedrive: the dev Pi with fake_drive.py standing in for the motor drive.

fake_drive.py creates a pty at the motor UART path and records every Waveshare frame the FSW
sends, so these tests check the exact CANopen SDO sequence end to end, and hot-plugging.
"""

import time

import pytest

from mara import (
    CONTROLWORD,
    MODE_OF_OPERATION,
    MOTOR,
    PROFILE_VELOCITY,
    SEQ,
    TARGET_POSITION,
    UART,
)

pytestmark = pytest.mark.fakedrive

ENABLE = [
    (MODE_OF_OPERATION, 1),
    (CONTROLWORD, 0x80),
    (CONTROLWORD, 0x06),
    (CONTROLWORD, 0x07),
    (CONTROLWORD, 0x0F),
]


def move(target, velocity=None):
    """The SDO writes of one move."""
    head = [(PROFILE_VELOCITY, velocity)] if velocity else []
    return head + [(CONTROLWORD, 0x0F), (TARGET_POSITION, target & 0xFFFFFFFF), (CONTROLWORD, 0x3F)]


@pytest.fixture
def connected(mara, fake_drive):
    """Fake drive plugged in and the FSW connected to it."""
    mara.await_event(f"{UART}.AdapterConnected", timeout=5)
    mara.set_motor_params(min_pos=-1000, max_pos=1000)
    fake_drive.clear()
    return fake_drive


def test_enable_sequence(mara, connected):
    mara.orch("enterTestMode")
    assert connected.wait_for(len(ENABLE)) == ENABLE
    mara.await_event(f"{MOTOR}.Enabled")


def test_move_frames(mara, connected):
    mara.orch("enterTestMode")
    connected.wait_for(len(ENABLE))
    connected.clear()

    mara.orch("testPlatformMoveTo", [500])
    assert connected.wait_for(3) == move(500)
    mara.await_event(f"{MOTOR}.MovingTo", [500])


def test_velocities(mara, connected):
    mara.prm_set(MOTOR, "ADVANCE_VELOCITY", 111)
    mara.prm_set(MOTOR, "RETRACT_VELOCITY", 222)
    mara.orch("enterTestMode")
    connected.wait_for(len(ENABLE))
    connected.clear()

    mara.orch("testPlatformMoveTo", [500])
    mara.orch("testPlatformMoveTo", [0])
    assert connected.wait_for(8) == move(500, 111) + move(0, 222)


def test_negative_target(mara, connected):
    mara.orch("enterTestMode")
    connected.wait_for(len(ENABLE))
    connected.clear()

    mara.orch("testPlatformMoveTo", [-300])
    assert connected.wait_for(3) == move(-300)


def test_stop_frame(mara, connected):
    mara.orch("enterTestMode")
    connected.wait_for(len(ENABLE))
    connected.clear()

    mara.orch("testPlatformStop")
    assert connected.wait_for(1) == [(CONTROLWORD, 0x010F)]


def test_exit_test_retracts(mara, connected):
    mara.orch("enterTestMode")
    mara.orch("testPlatformMoveTo", [500])
    connected.wait_for(len(ENABLE) + 3)
    connected.clear()

    mara.orch("exitTestMode")
    assert connected.wait_for(3) == move(0)


def test_hot_plug(mara, connected):
    """Unplug: one disconnect, commands fail cleanly. Replug: connected again, commands work."""
    mara.orch("enterTestMode")
    connected.wait_for(len(ENABLE))

    connected.stop()  # closes the pty and removes the link, like pulling the adapter
    mara.await_event(f"{UART}.AdapterDisconnected", timeout=5)
    mara.orch("testPlatformStop")
    mara.await_event(f"{MOTOR}.SendFailed", timeout=3)
    time.sleep(3)
    mara.api.assert_event_count(1, f"{UART}.AdapterDisconnected")
    mara.api.assert_event_count(1, f"{UART}.AdapterNotConnected")

    connected.start()
    connected.clear()
    mara.await_event(f"{UART}.AdapterConnected", timeout=5)
    mara.orch("testPlatformStop")
    assert connected.wait_for(1) == [(CONTROLWORD, 0x010F)]


def test_pad_check_sequence_frames(mara, connected, request):
    """The pad-check sequence: enable, move to 500, back to 0, and the exit retract."""
    from test_software import _deliver

    path = _deliver(request, mara.compile_sequence("pad_check", _tmp(request)))
    mara.cmd(f"{SEQ}.CS_RUN", [path, "NO_BLOCK"])
    mara.await_event(f"{SEQ}.CS_SequenceComplete", timeout=30)
    assert connected.wait_for(len(ENABLE) + 9) == ENABLE + move(500) + move(0) + move(0)


def _tmp(request):
    import tempfile
    from pathlib import Path

    return Path(tempfile.mkdtemp(prefix="mara_seq_"))
