module Mara {
    @ INA-228 Current and voltage sensor with I2C
    active component INA228 {

        @ Port for I2C bus communication
        output port busWriteRead: Drv.I2cWriteRead

        @ Port for I2C bus communication
        output port busWrite: Drv.I2c

        @ Sync port, grandfathered in from INA-303
        sync input port run: Svc.Sched

        @ Telemetry reading for current/voltage data
        telemetry Reading: INAData

        @ I2C error event, grandfathered in from INA-303
        event I2cError(
            address: U32,
            status: Drv.I2cStatus
        ) severity warning high format "I2C error on address {} with status {}" throttle 5

        @ Command to force a RESET
        async command RESET()

        @ I2CSensor SM instance
        state machine instance INAStateMachine: Mara.I2CSensorStateMachine

        @ Port for current time
        time get port timeCaller

        @ Enables command handling
        import Fw.Command

        @ Enables event handling
        import Fw.Event

        @ Enables Telemetry
        import Fw.Channel

        @data products for storing the current voltage sensor readings
        product record INARecord: INADataTimed id 0
        product container INAContainer id 0 default priority 10
        product get port productGetOut
        product send port productSendOut

        event DpMemoryFailure(allocationSize: FwSizeType) \
            severity warning high \
            format "Memory allocation of size {} for data product container for INA failed" \
            throttle 2
    }
}
