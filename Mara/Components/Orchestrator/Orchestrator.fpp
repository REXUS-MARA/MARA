module Mara {
    @ Orchestrator of the whole experiment
    active component Orchestrator {

        async command enterTestMode

        async command exitTestMode

        @ TEST mode: start the drill
        async command testDrillON

        @ TEST mode: stop the drill
        async command testDrillOFF

        @ TEST mode: move the platform to an absolute position in encoder counts.
        @ PlatformMotor clamps it to [MIN_POSITION, MAX_POSITION]. 0 is fully down.
        async command testPlatformMoveTo(position: I32)

        @ TEST mode: halt the platform where it is
        async command testPlatformStop

        async command testOpticalCameraON
        async command testOpticalCameraOFF

        async command testThermalCameraON
        async command testThermalCameraOFF

        state machine instance OrchestratorStateMachine : OrchestratorStateMachine

        async input port LOHigh: Fw.Signal
        async input port SOEHigh: Fw.Signal
        async input port EODSHigh: Fw.Signal

        @ 1 Hz tick, drives the experiment timeline
        async input port schedIn: Svc.Sched

        # ------------------------------------------------------------------
        # Experiment timeline parameters
        # ------------------------------------------------------------------

        @ Seconds the drill spins before the platform starts to advance
        param SPIN_UP_SECONDS: U32 default 3

        @ Seconds the platform advances (drills) before the drill stops and the platform retracts
        param ADVANCE_SECONDS: U32 default 60

        @ Seconds allowed for the retract before the timeline is marked DONE
        param RETRACT_SECONDS: U32 default 60

        @ Platform target for drilling, in encoder counts.
        @ TODO: set after commissioning. Also limited by PlatformMotor MAX_POSITION.
        param DRILL_POSITION: I32 default 0

        # ------------------------------------------------------------------
        # Events
        # ------------------------------------------------------------------

        event EODSTestSignal severity activity high format "Detected EODS signal in TEST mode, disregarding."
        event LOTestSignal severity activity high format "Detected LO signal in TEST mode, disregarding."
        event SOETestSignal severity activity high format "Detected SOE signal in TEST mode, disregarding."
        event EnterTest severity activity high format "Entered TEST state."
        event ExitTest severity activity high format "Exit TEST state, back to IDLE."
        event EnterFlight severity activity high format "Entered Flight state."
        event EnterExperiment severity activity high format "Entered Experiment state."
        event EnterAfterExperiment severity activity high format "Entered AfterExperiment state."
        event EnterSafe severity activity high format "Entered Safe state."

        @ DRILL_POSITION is 0 when the experiment starts
        event DrillPositionUnset \
            severity warning high \
            format "DRILL_POSITION is 0, the platform will not move during the experiment"

        @ A test command was sent outside TEST mode and was not executed
        event TestCommandIgnored \
            severity warning low \
            format "Not in TEST mode, command ignored"

        @ enterTestMode is only accepted in IDLE
        event EnterTestIgnored \
            severity warning low \
            format "Not in IDLE, enterTestMode ignored"

        @ exitTestMode is only accepted in TEST
        event ExitTestIgnored \
            severity warning low \
            format "Not in TEST mode, exitTestMode ignored"

        event PhaseSpinUp(seconds: U32) \
            severity activity high \
            format "Experiment: drill ON, spinning up for {} s"

        event PhaseAdvance(position: I32, seconds: U32) \
            severity activity high \
            format "Experiment: platform advancing to {} counts for {} s"

        event PhaseRetract(seconds: U32) \
            severity activity high \
            format "Experiment: drill OFF, platform retracting, allowing {} s"

        event PhaseDone \
            severity activity high \
            format "Experiment: drilling timeline done"

        # ------------------------------------------------------------------
        # Telemetry
        # ------------------------------------------------------------------

        @ Seconds since the current experiment phase began
        telemetry PhaseSeconds: U32

        # ------------------------------------------------------------------
        # Hardware ports
        # ------------------------------------------------------------------

        output port OpticalCameraON: Fw.Signal
        output port OpticalCameraOFF: Fw.Signal

        @ Drill control. Not connected yet: there is no drill component.
        output port DrillON: Fw.Signal
        output port DrillOFF: Fw.Signal

        output port PlatformEnable: Fw.Signal
        output port PlatformMoveTo: PlatformMoveTo
        output port PlatformStop: Fw.Signal

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
