module Mara {
    @ Experiment state machine
    state machine OrchestratorStateMachine {
        @ Initial state after boot
        initial enter IDLE

        @Lift off signal
        signal LO

        @ Start of the experiment signal
        @ Either received from REXUS SM or emitted after timer passes
        signal SOE

        @ End of data storage signal
        @ received from the REXUS SM or emitted early by us
        signal EODS

        signal EnterTest

        @ That could be done with ToggleTest
        @ But that's less expressive, we want to be explicit here
        signal ExitTest

        @ Unrecoverable error
        signal error

        @ 1 Hz tick from the rate group, drives the experiment timeline
        signal tick

        @ test the main drill ON
        signal testDrillON

        @ test the main drill OFF
        signal testDrillOFF

        @ test the platform: move to an absolute position (encoder counts)
        signal testPlatformMoveTo: I32

        @ test the platform: halt
        signal testPlatformStop

        @ test the optical camera ON
        signal testOpticalCameraON

        @ test the optical camera OFF
        signal testOpticalCameraOFF

        @ test the thermal camera ON
        signal testThermalCameraON

        @ test the thermal camera OFF
        signal testThermalCameraOFF

        # ------------------------------------------------------------------
        # Hardware actions (shared by TEST and the flight timeline)
        # ------------------------------------------------------------------

        action drillOn
        action drillOff
        action cameraOn
        action cameraOff
        action platformEnable
        @ Move the platform up to DRILL_POSITION
        action platformAdvance
        @ Move the platform back down to 0
        action platformRetract
        action platformStop

        # ------------------------------------------------------------------
        # TEST-only actions
        # ------------------------------------------------------------------

        @ test the platform: move to an absolute position
        action doTestPlatformMoveTo: I32

        @ test the thermal camera ON
        action doTestThermalCameraON

        @ test the thermal camera OFF
        action doTestThermalCameraOFF

        @ test the LO
        action doTestLO

        @ test the SOE
        action doTestSOE

        @ test the EODS
        action doTestEODS

        # ------------------------------------------------------------------
        # Experiment timeline
        # ------------------------------------------------------------------

        @ Read the timeline parameters once, so the running timeline can't change
        action loadTimeline

        @ Restart the per-phase seconds counter
        action resetPhaseTimer

        @ SPIN_UP_SECONDS have passed in SPIN_UP
        guard spinUpDone

        @ ADVANCE_SECONDS have passed in ADVANCE
        guard advanceDone

        @ RETRACT_SECONDS have passed in RETRACTING (only marks the end of the timeline)
        guard retractDone

        action notifyEnterTEST
        action notifyExitTEST
        action notifyEnterFlight
        action notifyEnterExperiment
        action notifySpinUp
        action notifyAdvance
        action notifyRetract
        action notifyDone
        action notifyEnterAfterExperiment
        action notifyEnterSafe

        @ Initial state
        state IDLE {
            on LO enter FLIGHT
            on EnterTest enter TEST
        }

        @ On the ground, connected to the rocket: hardware is driven by ground commands.
        @ LO/SOE/EODS are deliberately only logged here, at ESA's request for this flight
        @ (against the usual practice of leaving TEST on LO). The operator must send
        @ exitTestMode before launch, or the experiment will not run.
        state TEST {
            entry do { notifyEnterTEST, platformEnable }
            on ExitTest enter IDLE
            on LO do { doTestLO }
            on SOE do { doTestSOE }
            on EODS do { doTestEODS }
            on testDrillON do { drillOn }
            on testDrillOFF do { drillOff }
            on testPlatformMoveTo do { doTestPlatformMoveTo }
            on testPlatformStop do { platformStop }
            on testOpticalCameraON do { cameraOn }
            on testOpticalCameraOFF do { cameraOff }
            on testThermalCameraON do { doTestThermalCameraON }
            on testThermalCameraOFF do { doTestThermalCameraOFF }
            @ Leave in a known state: drill and camera off, platform back to 0.
            @ The retract is a move to 0 rather than a halt, so it never freezes mid-way.
            exit do { drillOff, cameraOff, platformRetract, notifyExitTEST }
        }

        @ After lift-off, waiting for the start of the experiment
        state FLIGHT {
            entry do { notifyEnterFlight }
            on SOE enter EXPERIMENT
            on error enter SAFE
        }

        @ Autonomous experiment timeline:
        @ SPIN_UP -> ADVANCE -> RETRACTING -> DONE
        @ Cameras record from here until AFTER_EXPERIMENT.
        state EXPERIMENT {
            entry do { loadTimeline, notifyEnterExperiment, cameraOn, platformEnable }
            initial enter DRILLING
            on EODS enter AFTER_EXPERIMENT
            on error enter SAFE

            @ Drill is spinning. Leaving this state for any reason
            @ (timer, EODS, error) stops the drill and lowers the platform.
            state DRILLING {
                exit do { drillOff, platformRetract }
                initial enter SPIN_UP

                @ Drill spins up while the platform waits at the bottom
                state SPIN_UP {
                    entry do { notifySpinUp, resetPhaseTimer, drillOn }
                    on tick if spinUpDone enter ADVANCE
                }

                @ Platform feeds the drill into the sample
                state ADVANCE {
                    entry do { notifyAdvance, resetPhaseTimer, platformAdvance }
                    on tick if advanceDone enter RETRACTING
                }
            }

            @ Drill is off, platform is going down (commanded on DRILLING exit)
            state RETRACTING {
                entry do { notifyRetract, resetPhaseTimer }
                on tick if retractDone enter DONE
            }

            @ Timeline finished. The drive holds the platform at its target by itself
            @ (no halt here, so a slow retract is never cut short). Cameras keep recording.
            state DONE {
                entry do { notifyDone }
            }
        }

        @ End of data storage: close the recordings
        state AFTER_EXPERIMENT {
            entry do { notifyEnterAfterExperiment, cameraOff }
            on error enter SAFE
        }

        @ In this state, just hunker down
        state SAFE {
            entry do { notifyEnterSafe }
        }
    }
}
