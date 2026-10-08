module Mara {
    @ BMP280 pressure and temperature sensor over I2C, using the Bosch BMP2 Sensor API
    queued component Bmp280Manager {

        # ==============================================================
        # Ports
        # ==============================================================

        @ Rate group tick: advances the sensor state machine
        sync input port run: Svc.Sched

        @ Port for I2C register reads
        output port busWriteRead: Drv.I2cWriteRead

        @ Port for I2C register writes
        output port busWrite: Drv.I2c

        # ==============================================================
        # State machine
        # ==============================================================

        @ Reset/enable/configure/run sequence shared by the I2C sensors
        state machine instance sensorSm: Mara.I2CSensorStateMachine drop

        # ==============================================================
        # Commands
        # ==============================================================

        @ Force a sensor reset
        async command RESET() drop

        # ==============================================================
        # Telemetry
        # ==============================================================

        @ Last compensated reading
        telemetry Reading: Bmp280Data

        @ Bosch compensation result of the last reading (0 OK, 1..4 clamped, <0 error)
        telemetry Status: I8

        # ==============================================================
        # Events
        # ==============================================================

        event I2cError(
            address: U8
            status: Drv.I2cStatus
        ) severity warning high format "BMP280 I2C error on address 0x{x} with status {}" throttle 5

        event ChipIdMismatch(
            chipId: U8
        ) severity warning high format "BMP280 unexpected chip ID 0x{x} (expected 0x58)" throttle 5

        event BoschError(
            code: I8
        ) severity warning high format "BMP280 Bosch API error {}" throttle 5

        event BadReading(
            code: I8
        ) severity warning low format "BMP280 raw reading out of ADC range (Bosch code {})" throttle 5

        event OutOfSpec(
            code: I8
        ) severity warning low format "BMP280 reading clamped to the sensor range (Bosch code {}); raw ADC values are kept in the data product" throttle 2

        event Configured(
            measurementTimeUs: U32
        ) severity activity high format "BMP280 configured, measurement time {} us"

        event DpMemoryFailure(
            allocationSize: FwSizeType
        ) severity warning high format "Memory allocation of size {} for BMP280 data product failed" throttle 2

        # ==============================================================
        # Parameters
        # ==============================================================

        @ I2C device address (0x76 with SDO low, 0x77 with SDO high)
        param I2C_ADDRESS: U8 default 0x77

        @ Oversampling preset
        param OVERSAMPLING: Bmp280OsMode default Bmp280OsMode.STANDARD_RESOLUTION

        @ IIR filter coefficient
        param FILTER: Bmp280Filter default Bmp280Filter.OFF

        @ Standby time between measurements in normal mode
        param STANDBY: Bmp280Standby default Bmp280Standby.MS_0_5

        # ==============================================================
        # Data products
        # ==============================================================

        @ Sensor trimming parameters, first record of every container
        product record CalibRecord: Bmp280Calib id 0

        @ Sensor configuration, second record of every container
        product record ConfigRecord: Bmp280Config id 2

        @ One reading per rate group tick
        product record ReadingRecord: Bmp280DataTimed id 1

        product container ReadingContainer id 0 default priority 10

        product get port productGetOut

        product send port productSendOut

        # ==============================================================
        # Standard AC Ports
        # ==============================================================

        @ Port for requesting the current time
        time get port timeCaller

        @ Enables command handling
        import Fw.Command

        @ Enables event handling
        import Fw.Event

        @ Enables telemetry channels handling
        import Fw.Channel

        @ Port to return the value of a parameter
        param get port prmGetOut

        @ Port to set the value of a parameter
        param set port prmSetOut
    }
}
