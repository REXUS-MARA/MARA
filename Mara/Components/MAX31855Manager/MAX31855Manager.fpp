
module Mara {
    @ thermocoupler adc
    passive component MAX31855Manager {

        ##############################################################################
        #### Uncomment the following examples to start customizing your component ####
        ##############################################################################

        # @ Example async command
        # async command COMMAND_NAME(param_name: U32)

        # @ Example telemetry counter
        # telemetry ExampleCounter: U64

        # @ Example event
        # event ExampleStateEvent(example_state: Fw.On) severity activity high id 0 format "State set to {}"

        # @ Example port: receiving calls from the rate group
        # sync input port run: Svc.Sched

        # @ Example parameter
        # param PARAMETER_NAME: U32

        sync input port run: Svc.Sched

        output port spiReadWrite: Drv.SpiReadWrite

        telemetry THERMOCOUPLE_TEMP: F32
        telemetry COLD_JUNCTION_TEMP: F32
        telemetry THERMOCOUPLE_FAULT: U8


        event ThermocoupleOpen severity activity high \
            format "Thermocouple open circuit detected"

        event ThermocoupleShortToGnd severity activity high \
            format "Thermocouple shorted to ground"

        event ThermocoupleShortToVcc severity activity high \
            format "Thermocouple shorted to VCC"

        event ThermocoupleFaultCleared severity activity high \
            format "Thermocouple fault cleared"


        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
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

        @Port to set the value of a parameter
        param set port prmSetOut

    }
}