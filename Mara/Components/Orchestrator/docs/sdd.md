# Mara::Orchestrator

Runs the experiment. It is an active component driven by `OrchestratorStateMachine.fpp`, which takes the REXUS service-module signals (LO, SOE, EODS) from `GPIOWatcher` and a 1 Hz tick (`schedIn`, rateGroup1).

## States
```
IDLE ──LO──> FLIGHT ──SOE──> EXPERIMENT ──EODS──> AFTER_EXPERIMENT
  │                            │ (error from FLIGHT/EXPERIMENT/AFTER_EXPERIMENT ──> SAFE)
  └─enterTestMode─> TEST ─exitTestMode─> IDLE
```

## Flight timeline (EXPERIMENT), fully autonomous
The timeline parameters are read once, on entry to EXPERIMENT. Each phase logs an event, and `PhaseSeconds` telemetry counts up within the phase.

| Phase | On entry | Leaves when |
|---|---|---|
| (EXPERIMENT entry) | optical camera ON, platform drive enable | — |
| `DRILLING.SPIN_UP` | drill ON, `PhaseSpinUp` | `SPIN_UP_SECONDS` elapsed |
| `DRILLING.ADVANCE` | platform → `DRILL_POSITION`, `PhaseAdvance` | `ADVANCE_SECONDS` elapsed |
| `RETRACTING` | `PhaseRetract` | `RETRACT_SECONDS` elapsed |
| `DONE` | `PhaseDone` | — (cameras keep recording) |
| AFTER_EXPERIMENT | optical camera OFF | — |

**Retract guarantee.** The exit action of `DRILLING` is *drill OFF + platform → 0*. It runs however DRILLING is left: by the timer, by EODS, or by an error (→ SAFE). So the platform always comes down once drilling has started. `DONE` does not halt the drive, so a retract that runs longer than `RETRACT_SECONDS` still finishes.

| Parameter | Default | |
|---|---|---|
| `SPIN_UP_SECONDS` | 3 | drill spin-up before advancing |
| `ADVANCE_SECONDS` | 60 | drilling time |
| `RETRACT_SECONDS` | 60 | time allowed for the retract (only marks DONE) |
| `DRILL_POSITION` | 0 | platform target in counts. **TODO**, and also limited by PlatformMotor `MAX_POSITION`. If it is 0 when the experiment starts, `DrillPositionUnset` (WARNING_HI) is raised and the timeline still runs |

## TEST mode (on the ground, connected to the rocket)
Entering TEST enables the platform drive. LO/SOE/EODS are only logged. This is deliberate, at ESA's request for this flight, against the usual practice of leaving TEST on LO. **Pre-launch step: send `exitTestMode`.** If the software is still in TEST at lift-off, the experiment will not run.

The test commands below only act in TEST. In any other state they are not executed: the Orchestrator logs `TestCommandIgnored` (WARNING_LO) and responds `EXECUTION_ERROR`, which also aborts a test sequence file that runs in the wrong state. Likewise `enterTestMode` is only accepted in IDLE (`EnterTestIgnored`) and `exitTestMode` only in TEST (`ExitTestIgnored`).

Leaving TEST puts the hardware in a known state: drill off, camera off, platform moving back to 0. The retract is a move to 0, not a halt, so it never stops mid-way.

| Command | Effect |
|---|---|
| `testDrillON` / `testDrillOFF` | drill (output ports not wired yet: no drill component) |
| `testPlatformMoveTo(position)` | absolute move in counts, clamped by PlatformMotor |
| `testPlatformStop` | halt the platform |
| `testOpticalCameraON` / `testOpticalCameraOFF` | optical camera recording |
| `testThermalCameraON` / `testThermalCameraOFF` | TODO |

Example test: `enterTestMode`, `testOpticalCameraON`, `testPlatformMoveTo 2000`, wait, `testPlatformMoveTo 0`, `testOpticalCameraOFF`, `exitTestMode`.

## Queue overflow
Every async port, command and state-machine signal uses the `drop` queue-full policy instead of the F´ default, which asserts (FATAL). A dropped message is silent apart from the internal dropped-message counter, and a dropped command gets no response. With a queue depth of 10 and a few messages per minute this is theoretical, but a drop is recoverable and a FATAL is not.
