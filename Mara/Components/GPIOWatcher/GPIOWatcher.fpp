module Mara {
    @ Wrapper around the gpio pin, so that we can debounce properly.
    passive component GPIOWatcher {
        
        @ For each pin we have 2 input ports
        @ One for the interrupt
        @ And one for reading so that we can do software debouncing
        sync input port LOPinInterrupt: Svc.Sched
        output port LOPinRead: Drv.GpioRead
        @ And then we have an output port
        @ To let the orchestrator know that we have a confirmed signal
        output port LOHigh: Fw.Signal
        @ And some events to inform if something of interest has happened
        event LOSpike severity activity high format "Detected a spike on the LO line"
        event LODetected severity activity high format "Detected LO signal."

        sync input port SOEPinInterrupt: Svc.Sched
        output port SOEPinRead: Drv.GpioRead
        output port SOEHigh: Fw.Signal
        event SOESpike severity activity high format "Detected a spike on the SOE line"
        event SOEDetected severity activity high format "Detected SOE signal."

        sync input port EODSPinInterrupt: Svc.Sched
        output port EODSPinRead: Drv.GpioRead
        output port EODSHigh: Fw.Signal
        event EODSSpike severity activity high format "Detected a spike on the EODS line"
        event EODSDetected severity activity high format "Detected EODS signal."


        @ Preferably this would be connected to a rate group fast enough
        @ To do debouncing quick
        @ It's not a requirement though
        @ Since if gpio signals are late we don't really care
        sync input port schedIn: Svc.Sched
        
        @ Port for requesting the current time
        time get port timeCaller

        @ Enables event handling
        import Fw.Event

        @ Enables telemetry channels handling
        import Fw.Channel

    }
}