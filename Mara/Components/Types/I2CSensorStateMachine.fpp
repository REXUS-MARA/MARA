module Mara {
    @ Define I2C Sensor State Machine
    @ Stolen from MpuImu from fprimesensors repo
    state machine I2CSensorStateMachine {
        @ Initial state: reset the device
        initial enter RESET

        @ Rate-group driven signal
        signal tick

        # @ Reconfigure signal
        # signal reconfigure

        @ Current state passed successfully
        signal success

        @ Current state erred
        signal error

        @ Perform reset commands
        action doReset

        @ Check reset took place
        action checkReset

        @ Perform enable commands
        action doEnable

        # @ Perform configure commands
        # action doConfigure

        @ Read the sensor
        action doRead

        @ Reset the sensor
        state RESET {
            on tick do { doReset }
            on success enter WAIT_RESET
        }

        state WAIT_RESET {
            on tick do { checkReset }
            on success enter ENABLE
            on error enter RESET
        }

        @ Enable sensor data flows
        state ENABLE {
            on tick do { doEnable }
            # on success enter CONFIGURE
            on success enter RUN
            on error enter RESET
        }

        # @ Configure sensor
        # state CONFIGURE {
        #     on tick do { doConfigure }
        #     on success enter RUN
        #     on error enter RESET
        # }

        @ Run the sensor
        state RUN {
            on tick do { doRead }
            # on reconfigure enter CONFIGURE
            on error enter RESET
        }
    }
}