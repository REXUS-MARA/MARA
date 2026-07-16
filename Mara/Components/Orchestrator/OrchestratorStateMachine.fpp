module Mara {
    @ Define Ltr State Machine
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

        @ test the main drill
        signal testDrill

        @ test the platform motors
        signal testPlatform

        @ test the main drill
        action doTestDrill

        @ test the platform motors
        action doTestPlatform

        @ test the LO
        action doTestLO

        @ test the SOE
        action doTestSOE

        @ test the EODS
        action doTestEODS

        @ Initial state
        state IDLE {
            on LO enter FLIGHT
            on EnterTest enter TEST
        }

        state TEST {
            on ExitTest enter IDLE
            on LO do { doTestLO }
            on SOE do { doTestSOE }
            on EODS do { doTestEODS }
            @ maybe we should put some guards around 
            @ testDrill and testPlatform
            on testDrill do { doTestDrill }
            on testPlatform do { doTestPlatform }
        }


        @ Wait for the sensor while it performs reset
        state FLIGHT {
            on SOE enter EXPERIMENT
            on error enter SAFE
        }

        @ Run the sensor
        state EXPERIMENT {
            on EODS enter AFTER_EXPERIMENT
            on error enter SAFE
        }

        @ Run the sensor
        state AFTER_EXPERIMENT {
            on error enter SAFE
        }

        @ In this state, just hunker down
        state SAFE {
        }
    }
}