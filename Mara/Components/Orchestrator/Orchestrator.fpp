module Mara {
    @ Orchestrator of the whole experiment
    active component Orchestrator {

        # One async command/port is required for active components
        # This should be overridden by the developers with a useful command/port
        @ TODO
        async command TODO opcode 0

        sync command enterTestMode

        sync command exitTestMode

        @ I'm not sure if those should be sync or async
        @ Because the test will take some time
        @ But do we expect it to end within some finite time
        @ It's a pickle for me
        sync command testDrill

        sync command testMotor

        state machine instance OrchestratorStateMachine : OrchestratorStateMachine

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