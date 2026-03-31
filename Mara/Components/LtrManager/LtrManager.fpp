module Mara {
    @ Used to get data from the light sensor LTR-303
    queued component LtrManager {

        @ Port for I2C bus communication
        output port busWriteRead: Drv.I2cWriteRead
        
        @ Port for I2C bus communication
        output port busWrite: Drv.I2c
        
        @ Port for requesting the current time
        @ Idk if that's needed, it was here by default
        time get port timeCaller

        @ Scheduling port for reading from IMU and writing to telemetry
        @ Stolen from the Mpu, maybe not needed? Idk
        sync input port run: Svc.Sched
        
        # @ Telemetry channel for the raw Ltr data
        telemetry Reading: LtrData

        @ Then they also defined a bunch of events for reconfiguring or configuring
        @ I'm not sure about those yet, cause I don't know what can be configured
        @ That seems like it could be outsourced somewhere
        @ It seems that event cannont be put into a common file?
        event I2cError(
            address: U32,
            status: Drv.I2cStatus
        ) severity warning high format "I2C error on address {} with status {}" throttle 5


        # Another important ToDo, they have a bunch of parameters for doing the configuration
        # But maybe I'll do that later
        @ Command to force a RESET
        async command RESET()

        @ I2CSensor SM instance
        state machine instance LtrStateMachine: Mara.I2CSensorStateMachine

        @ Enables command handling
        import Fw.Command

        @ Enables event handling
        import Fw.Event

        @ Enables telemetry channels handling
        import Fw.Channel

        @ Port to return the value of a parameter
        param get port prmGetOut

        @Port to set the value of a parameter
        param set port prmSetOut

    }
}