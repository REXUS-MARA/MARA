module Mara {

    @ Absolute platform target in encoder counts
    port PlatformMoveTo(position: I32)

    @ Moves the drill platform through a Technosoft iPOS drive (CANopen CiA 402,
    @ profile position mode), reached via a Waveshare USB-CAN-A adapter on a UART.
    @ Driven only by the Orchestrator, so the platform can't move outside TEST/EXPERIMENT.
    active component PlatformMotor {

        @ Serial link to the USB-CAN adapter (connects to a Drv.LinuxUartDriver)
        import Drv.PassiveByteStreamDriverClient

        @ Enable the drive. Also clears a drive fault.
        async input port enable: Fw.Signal

        @ Move to an absolute position in encoder counts, clamped to [MIN_POSITION, MAX_POSITION].
        @ 0 is the position at drive power-up.
        async input port moveTo: PlatformMoveTo

        @ Stop and hold position
        async input port stop: Fw.Signal

        @ Health ping
        async input port pingIn: Svc.Ping

        @ Health ping response
        output port pingOut: Svc.Ping

        # ------------------------------------------------------------------
        # Parameters
        # ------------------------------------------------------------------

        @ CANopen node ID of the drive, as set in EasySetUp
        param NODE_ID: U8 default 1

        @ Lowest allowed target, in encoder counts
        param MIN_POSITION: I32 default 0

        @ Highest allowed target, in encoder counts.
        @ TODO: set after commissioning (1 mm = 4 * encoder lines / 0.794 counts).
        @ Default 0 means the platform will not move until this is set.
        param MAX_POSITION: I32 default 0

        @ Profile velocity (0x6081) for upward moves, i.e. drilling feed. Technosoft internal units.
        @ 0 keeps the value stored in the drive.
        param ADVANCE_VELOCITY: U32 default 0

        @ Profile velocity (0x6081) for downward moves. Technosoft internal units.
        @ 0 keeps the value stored in the drive.
        param RETRACT_VELOCITY: U32 default 0

        @ Profile acceleration (0x6083), written on enable. Technosoft internal units.
        @ 0 keeps the value stored in the drive.
        param ACCELERATION: U32 default 0

        # ------------------------------------------------------------------
        # Events
        # ------------------------------------------------------------------

        event SendFailed(status: Drv.ByteStreamStatus) \
            severity warning high \
            format "Sending to the CAN adapter failed: {}"

        event Enabled \
            severity activity high \
            format "Platform drive enabled"

        event MovingTo(position: I32) \
            severity activity high \
            format "Platform moving to {} counts"

        event Stopped \
            severity activity high \
            format "Platform halted"

        event PositionClamped(requested: I32, clamped: I32) \
            severity warning low \
            format "Platform target {} outside limits, clamped to {}"

        # ------------------------------------------------------------------
        # Telemetry
        # ------------------------------------------------------------------

        @ Last commanded target position
        telemetry TargetPosition: I32

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Enables command handling (needed for parameters)
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
