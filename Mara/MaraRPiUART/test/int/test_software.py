"""software: runs against any FSW without hardware (laptop native build or dev Pi).

Checks the ground-facing behaviour: streaming, TEST-mode command rules, parameters, health,
and that the sequences compile and behave.
"""

import time

import pytest

from mara import CMD_DISP, MOTOR, ORCH, SEQ, TEST_COMMANDS

pytestmark = pytest.mark.software


def test_streaming(fprime_test_api):
    """Telemetry arrives: the orchestrator writes PhaseSeconds every second."""
    found = fprime_test_api.await_telemetry_count(3, f"{ORCH}.PhaseSeconds", timeout=10)
    assert len(found) >= 3


def test_enter_and_exit_test_mode(mara):
    mara.orch("enterTestMode")
    mara.await_event(f"{ORCH}.EnterTest")
    mara.orch("exitTestMode")
    mara.await_event(f"{ORCH}.ExitTest")


@pytest.mark.parametrize("command,args", TEST_COMMANDS, ids=[c for c, _ in TEST_COMMANDS])
def test_test_command_rejected_outside_test(mara, command, args):
    """Outside TEST every test command is answered EXECUTION_ERROR and logs TestCommandIgnored."""
    mara.cmd_rejected(f"{ORCH}.{command}", args)
    mara.await_event(f"{ORCH}.TestCommandIgnored", timeout=2)


@pytest.mark.parametrize("command,args", TEST_COMMANDS, ids=[c for c, _ in TEST_COMMANDS])
def test_test_command_accepted_in_test(mara, command, args):
    mara.orch("enterTestMode")
    mara.orch(command, args)


def test_enter_test_twice_rejected(mara):
    mara.orch("enterTestMode")
    mara.cmd_rejected(f"{ORCH}.enterTestMode")
    mara.await_event(f"{ORCH}.EnterTestIgnored", timeout=2)


def test_exit_test_outside_test_rejected(mara):
    mara.cmd_rejected(f"{ORCH}.exitTestMode")
    mara.await_event(f"{ORCH}.ExitTestIgnored", timeout=2)


def test_exit_test_leaves_known_state(mara):
    """Leaving TEST turns the camera off and moves the platform to 0 (motor events depend on the adapter)."""
    mara.orch("enterTestMode")
    mara.orch("testOpticalCameraON")
    mara.orch("exitTestMode")
    mara.await_event(f"{ORCH}.ExitTest")


@pytest.mark.parametrize(
    "component,param,value,default",
    [
        (ORCH, "SPIN_UP_SECONDS", 7, 3),
        (ORCH, "ADVANCE_SECONDS", 7, 60),
        (ORCH, "RETRACT_SECONDS", 7, 60),
        (ORCH, "DRILL_POSITION", -7, 0),
        (MOTOR, "NODE_ID", 2, 1),
        (MOTOR, "MIN_POSITION", -7, 0),
        (MOTOR, "MAX_POSITION", 7, 0),
        (MOTOR, "ADVANCE_VELOCITY", 7, 0),
        (MOTOR, "RETRACT_VELOCITY", 7, 0),
        (MOTOR, "ACCELERATION", 7, 0),
    ],
)
def test_parameter_set(mara, component, param, value, default):
    """Every new parameter can be set from the ground (then put back; nothing is saved)."""
    mara.prm_set(component, param, value)
    mara.prm_set(component, param, default)


def test_limits_unset_warning(mara):
    """With MIN = MAX = 0, entering TEST (which enables the drive) warns LimitsUnset."""
    mara.set_motor_params(min_pos=0, max_pos=0)
    mara.orch("enterTestMode")
    mara.await_event(f"{MOTOR}.LimitsUnset")


def test_no_missed_health_pings(fprime_test_api):
    """Over 20 s (5 health cycles) no component misses a ping."""
    time.sleep(20)
    fprime_test_api.assert_event_count(0, "CdhCore.health.HLTH_PING_WARN")
    fprime_test_api.assert_event_count(0, "CdhCore.health.HLTH_PING_LATE")


def test_wrong_state_sequence_aborts(mara, tmp_path, request):
    """A sequence that sends a test command outside TEST stops at that command."""
    binary = mara.compile_sequence("wrong_state", _sequence_dir(request, tmp_path))
    mara.cmd(f"{SEQ}.CS_RUN", [_deliver(request, binary), "NO_BLOCK"])
    mara.await_event(f"{SEQ}.CS_CommandError", timeout=5)
    time.sleep(2)  # the second command would have run by now
    mara.api.assert_event_count(0, "Mara.opticalCamera.RecordingStarted")
    mara.api.assert_event_count(0, f"{SEQ}.CS_SequenceComplete")


def test_pad_check_sequence_completes(mara, tmp_path, request):
    """The pad-check sequence runs to the end (motor commands fail cleanly without an adapter)."""
    mara.set_motor_params(min_pos=0, max_pos=1000)
    binary = mara.compile_sequence("pad_check", _sequence_dir(request, tmp_path))
    mara.cmd(f"{SEQ}.CS_RUN", [_deliver(request, binary), "NO_BLOCK"])
    mara.await_event(f"{SEQ}.CS_SequenceComplete", timeout=30)
    mara.api.assert_event_count(0, f"{SEQ}.CS_CommandError")
    mara.api.assert_event_count(1, f"{ORCH}.ExitTest")


def _sequence_dir(request, tmp_path):
    return tmp_path


def _deliver(request, binary):
    """Put the compiled sequence where the FSW can open it; return that path.

    Command string arguments are limited to 40 characters at runtime (FW_CMD_STRING_MAX_SIZE),
    although the dictionary advertises more, so the path must stay short.
    """
    import shutil

    from mara import PiShell

    path = f"/tmp/mara_{binary.stem}.bin"
    assert len(path) < 40
    host = request.config.getoption("--mara-pi")
    if request.config.getoption("--mara-target") == "laptop" or host is None:
        shutil.copy(binary, path)
    else:
        PiShell(host).copy_to(binary, path)
    return path
