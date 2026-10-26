"""gpio: flight-timeline rehearsal on the dev Pi.

Spare GPIO outputs are wired to the LO/SOE/EODS inputs (BCM 27/22/17) and driven with gpioset,
playing the REXUS service module. The fake drive records what the platform is told to do.
Each test restarts the FSW, the only way back to IDLE after LO.
"""

import pytest

from mara import CONTROLWORD, GPIO, MODE_OF_OPERATION, MOTOR, ORCH, PROFILE_VELOCITY, TARGET_POSITION, UART

pytestmark = pytest.mark.gpio

HOLD = 0.5  # seconds a line is held HIGH: 10 ticks at 20 Hz, well over the 4-tick debounce
SPIKE = 0.1  # 2 ticks: below the debounce
TOLERANCE = 1.5  # seconds: the timeline runs on a free-running 1 Hz tick


def seconds(event):
    time = event.get_time()
    return time.seconds + time.useconds / 1e6


@pytest.fixture
def flight_ready(mara, restart_fsw, fake_drive):
    """Fresh FSW in IDLE, connected to the fake drive, with short timeline parameters."""
    restart_fsw()
    mara.await_event(f"{UART}.AdapterConnected", timeout=5)
    mara.set_motor_params(min_pos=0, max_pos=1000, advance=111, retract=222)
    mara.api.clear_histories()
    fake_drive.clear()
    return fake_drive


def test_full_timeline(mara, gpio, flight_ready):
    mara.set_timeline(spin_up=3, advance=5, retract=4, drill_position=500)
    flight_ready.clear()

    gpio.pulse("LO", HOLD)
    mara.await_event(f"{ORCH}.EnterFlight", timeout=3)
    gpio.pulse("SOE", HOLD)
    mara.await_event(f"{ORCH}.EnterExperiment", timeout=3)

    spin = mara.await_event(f"{ORCH}.PhaseSpinUp", [3], timeout=3)
    advance = mara.await_event(f"{ORCH}.PhaseAdvance", [500, 5], timeout=3 + TOLERANCE + 2)
    retract = mara.await_event(f"{ORCH}.PhaseRetract", [4], timeout=5 + TOLERANCE + 2)
    done = mara.await_event(f"{ORCH}.PhaseDone", timeout=4 + TOLERANCE + 2)

    assert abs(seconds(advance) - seconds(spin) - 3) <= TOLERANCE
    assert abs(seconds(retract) - seconds(advance) - 5) <= TOLERANCE
    assert abs(seconds(done) - seconds(retract) - 4) <= TOLERANCE

    # What the platform was told: enable, advance at ADVANCE_VELOCITY, retract at RETRACT_VELOCITY
    sdos = flight_ready.wait_for(13)
    assert sdos[0] == (MODE_OF_OPERATION, 1)
    assert sdos[4] == (CONTROLWORD, 0x0F)
    assert sdos[5:9] == [(PROFILE_VELOCITY, 111), (CONTROLWORD, 0x0F), (TARGET_POSITION, 500), (CONTROLWORD, 0x3F)]
    assert sdos[9:13] == [(PROFILE_VELOCITY, 222), (CONTROLWORD, 0x0F), (TARGET_POSITION, 0), (CONTROLWORD, 0x3F)]
    mara.api.assert_event_count(0, f"{MOTOR}.SendFailed")


def test_eods_during_advance_retracts(mara, gpio, flight_ready):
    mara.set_timeline(spin_up=1, advance=60, retract=10, drill_position=500)
    gpio.pulse("LO", HOLD)
    gpio.pulse("SOE", HOLD)
    mara.await_event(f"{ORCH}.PhaseAdvance", timeout=5)
    flight_ready.wait_for(9)
    flight_ready.clear()

    gpio.pulse("EODS", HOLD)
    mara.await_event(f"{ORCH}.EnterAfterExperiment", timeout=3)
    assert flight_ready.wait_for(4) == [
        (PROFILE_VELOCITY, 222),
        (CONTROLWORD, 0x0F),
        (TARGET_POSITION, 0),
        (CONTROLWORD, 0x3F),
    ]
    mara.api.assert_event_count(0, f"{ORCH}.PhaseRetract")  # straight to AFTER_EXPERIMENT


def test_lo_spike_ignored(mara, gpio, flight_ready):
    gpio.pulse("LO", SPIKE)
    mara.await_event(f"{GPIO}.LOSpike", timeout=3)
    mara.api.assert_event_count(0, f"{GPIO}.LODetected")
    mara.api.assert_event_count(0, f"{ORCH}.EnterFlight")


def test_lo_only_logged_in_test_then_flight_after_exit(mara, gpio, flight_ready):
    """ESA requirement: LO in TEST is only logged. Pre-launch procedure: exitTestMode, then LO works."""
    mara.orch("enterTestMode")
    gpio.pulse("LO", HOLD)
    mara.await_event(f"{ORCH}.LOTestSignal", timeout=3)
    mara.api.assert_event_count(0, f"{ORCH}.EnterFlight")
    mara.orch("testPlatformStop")  # accepted: still in TEST

    mara.orch("exitTestMode")
    gpio.pulse("LO", HOLD)
    mara.await_event(f"{ORCH}.EnterFlight", timeout=3)
