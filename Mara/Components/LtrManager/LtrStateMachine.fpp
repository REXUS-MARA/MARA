module Mara {
    @ Define Ltr State Machine
    @ Stolen from MpuImu from fprimesensors repo
    state machine LtrStateMachine {
        @ Initial state: reset the device
        initial enter RESET

        @ Rate-group driven signal
        signal tick

        # @ Reconfigure signal
        signal reconfigure

        @ Current state passed successfully
        signal success

        @ Current state erred
        signal error

        @ Perform reset commands
        action doReset

        @ Check reset took place
        action checkReset

        @ Perform configure commands
        action doConfigure

        @ Read the sensor
        action doRead

        @ Reset the sensor
        state RESET {
            on tick do { doReset }
            on success enter WAIT_RESET
        }

        @ Wait for the sensor while it performs reset
        state WAIT_RESET {
            on tick do { checkReset }
            on success enter CONFIGURE
            on error enter RESET
        }

        @ Configure sensor
        state CONFIGURE {
            on tick do { doConfigure }
            on success enter RUN
            on error enter RESET
        }

        @ Run the sensor
        state RUN {
            on tick do { doRead }
            on reconfigure enter CONFIGURE
            on error enter RESET
        }
    }
}