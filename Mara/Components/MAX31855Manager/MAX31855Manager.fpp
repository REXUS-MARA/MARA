module Mara {
    @ MAX31855 thermocouple-to-digital converter over SPI
    passive component MAX31855Manager {

        # ==============================================================
        # Ports
        # ==============================================================

        @ Port for receiving calls from the rate group (periodic reads)
        sync input port run: Svc.Sched

        @ SPI port for communicating with the MAX31855
        output port spiWriteRead: Drv.SpiWriteRead

        # ==============================================================
        # Telemetry
        # ==============================================================

        telemetry thermocoupleTemp: F32

        telemetry internalTemp: F32

        @ Fault bits: 0x1 open circuit, 0x2 short to GND, 0x4 short to VCC
        telemetry faults: U8 format "0x{x}"

        @ Full reading
        telemetry temperature: ThermoData

        # ==============================================================
        # Events
        # ==============================================================

        @ Sensor returned its first valid frame
        event MAX31855_INITIALIZED \
            severity activity high \
            format "MAX31855 initialized successfully"

        @ SPI transfer error
        event MAX31855_SPI_ERROR(
            errStatus: I32
        ) severity warning high \
            format "MAX31855 SPI error: status {}" \
            throttle 5

        @ Frame with reserved bits set (usually no device / MISO floating)
        event MAX31855_BAD_FRAME(
            frame: U32
        ) severity warning high \
            format "MAX31855 invalid frame: 0x{x}" \
            throttle 5

        @ Thermocouple fault detected or fault type changed
        event MAX31855_FAULT(
            faults: U8
        ) severity warning high \
            format "MAX31855 thermocouple fault: 0x{x} (0x1 open, 0x2 short GND, 0x4 short VCC)"

        @ Thermocouple fault cleared
        event MAX31855_FAULT_CLEARED \
            severity activity high \
            format "MAX31855 thermocouple fault cleared"

        event DpMemoryFailure(allocationSize: FwSizeType) \
            severity warning high \
            format "Memory allocation of size {} for MAX31855 data product failed" \
            throttle 2

        # ==============================================================
        # Data products
        # ==============================================================

        product record ThermoRecord: ThermoDataTimed id 0
        product container ThermoContainer id 0 default priority 10
        product get port productGetOut
        product send port productSendOut

        # ==============================================================
        # Standard AC Ports
        # ==============================================================

        @ Port for requesting the current time
        time get port timeCaller

        @ Enables event handling
        import Fw.Event

        @ Enables telemetry channels handling
        import Fw.Channel
    }
}
