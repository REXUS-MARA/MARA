module Mara {
    @ Wrapper around the gpio pin, so that we can debounce properly.
    passive component GPIOWatcher {
        
        output port LOPinRead: Drv.GpioRead
        @ And then we have an output port
        @ To let the orchestrator know that we have a confirmed signal
        output port LOHigh: Fw.Signal
        @ And some events to inform if something of interest has happened
        event LOSpike severity activity high format "Detected a spike on the LO line"
        event LODetected severity activity high format "Detected LO signal."

        output port SOEPinRead: Drv.GpioRead
        output port SOEHigh: Fw.Signal
        event SOESpike severity activity high format "Detected a spike on the SOE line"
        event SOEDetected severity activity high format "Detected SOE signal."

        output port EODSPinRead: Drv.GpioRead
        output port EODSHigh: Fw.Signal
        event EODSSpike severity activity high format "Detected a spike on the EODS line"
        event EODSDetected severity activity high format "Detected EODS signal."

        @ Shows the state of signals for the ground station
        enum SignalState { NOT_DETECTED, DETECTED }

        telemetry LOState:   SignalState
        telemetry SOEState:  SignalState
        telemetry EODSState: SignalState


        @ Which GPIO line a record refers to
        enum GpioPin { EODS, LO, SOE }

        @ One pin's first detected flip
        struct PinFlipRecord {
            pin: GpioPin
            seconds: U32
            useconds: U32
        }

        # Data product ports (synchronous get, works for a passive component)
        product get port productGetOut
        product send port productSendOut

        @ Record holding a pin flip
        product record PinFlip: PinFlipRecord id 0

        @ Container for first-flip records
        product container PinFlips id 0 default priority 10

        @ Could not get a data product buffer
        event PinFlipDpUnavailable(pin: GpioPin) \
        severity warning low \
        format "No DP buffer available to record first flip of {}" \
        throttle 5

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