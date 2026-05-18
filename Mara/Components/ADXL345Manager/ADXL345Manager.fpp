module Mara {
    @ ADXL345 accelerometer sensor over I2C
    passive component ADXL345Manager {
        # ==============================================================
        # Ports
        # ==============================================================
        @ Port for receiving calls from the rate group (periodic reads)
        sync input port run: Svc.Sched
        @ I2C port for communicating with the ADXL345
        output port i2cReadWrite: Drv.I2cWriteRead
        output port i2cWrite: Drv.I2c
        # ==============================================================
        # Commands
        # ==============================================================
        @ Set the measurement range (0=2g, 1=4g, 2=8g, 3=16g)
        sync command ADXL345_SET_RANGE(
            range: U8
        ) opcode 0x00
        @ Set the data rate (e.g. 0x0A = 100Hz)
        sync command ADXL345_SET_RATE(
            rate: U8
        ) opcode 0x01
        # ==============================================================
        # Telemetry
        # ==============================================================
        @ X-axis acceleration in g
        telemetry accelX: F32
        @ Y-axis acceleration in g
        telemetry accelY: F32
        @ Z-axis acceleration in g
        telemetry accelZ: F32
        # ==============================================================
        # Events
        # ==============================================================
        @ Sensor initialized successfully
        event ADXL345_INITIALIZED \
            severity activity high \
            format "ADXL345 initialized successfully"
        @ Sensor initialization failed
        event ADXL345_INIT_FAILED(
            errStatus: I32
        ) severity warning high \
            format "ADXL345 initialization failed with status {}"
        @ I2C read/write error
        event ADXL345_I2C_ERROR(
            errStatus: I32
        ) severity warning high \
            format "ADXL345 I2C error: status {}"
        @ Device ID mismatch
        event ADXL345_BAD_DEVICE_ID(
            deviceId: U8
        ) severity warning high \
            format "ADXL345 unexpected device ID: 0x{x}"

        event DpMemoryFailure(allocationSize: FwSizeType) \
            severity warning high \
            format "Memory allocation of size {} for ADXL345 data product failed" \
            throttle 2
        # ==============================================================
        # Parameters
        # ==============================================================
        @ I2C device address (default 0x53)
        param I2C_ADDR: U8 default 0x53
        @ Measurement range (0=2g, 1=4g, 2=8g, 3=16g)
        param RANGE: U8 default 0
        product record AccelRecord: AccelDataTimed id 0
        product container AccelContainer id 0 default priority 10
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