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
| `DRILL_POSITION` | 0 | platform target in counts. **TODO**, and also limited by PlatformMotor `MAX_POSITION` |

## TEST mode (on the ground, connected to the rocket)
Entering TEST enables the platform drive. LO/SOE/EODS are only logged. The test commands below are accepted in every state but act only in TEST, so ground commands can't move hardware in flight.

| Command | Effect |
|---|---|
| `testDrillON` / `testDrillOFF` | drill (output ports not wired yet: no drill component) |
| `testPlatformMoveTo(position)` | absolute move in counts, clamped by PlatformMotor |
| `testPlatformStop` | halt the platform |
| `testOpticalCameraON` / `testOpticalCameraOFF` | optical camera recording |
| `testThermalCameraON` / `testThermalCameraOFF` | TODO |

Example test: `enterTestMode`, `testOpticalCameraON`, `testPlatformMoveTo 2000`, wait, `testPlatformMoveTo 0`, `testOpticalCameraOFF`, `exitTestMode`.
