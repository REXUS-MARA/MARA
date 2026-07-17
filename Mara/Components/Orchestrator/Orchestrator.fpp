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