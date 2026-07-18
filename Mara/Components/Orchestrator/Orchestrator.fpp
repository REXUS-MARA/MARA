module Mara {
    @ Orchestrator of the whole experiment
    active component Orchestrator {

        async command enterTestMode

        async command exitTestMode

        @ I'm not sure if those should be sync or async
        @ Because the test will take some time
        @ But do we expect it to end within some finite time
        @ It's a pickle for me
        async command testDrill

        async command testPlatform

        state machine instance OrchestratorStateMachine : OrchestratorStateMachine

        async input port LOHigh: Fw.Signal
        async input port SOEHigh: Fw.Signal
        async input port EODSHigh: Fw.Signal

        event EODSTestSignal severity activity high format "Detected EODS signal in TEST mode, disregarding."
        event LOTestSignal severity activity high format "Detected LO signal in TEST mode, disregarding."
        event SOETestSignal severity activity high format "Detected SOE signal in TEST mode, disregarding."
        event EnterTest severity activity high format "Entered TEST state."
        event ExitTest severity activity high format "Exit TEST state, back to IDLE."
        event EnterFlight severity activity high format "Entered Flight state."
        event EnterExperiment severity activity high format "Entered Experiment state."
        event EnterAfterExperiment severity activity high format "Entered AfterExperiment state."
        event EnterSafe severity activity high format "Entered Safe state."


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