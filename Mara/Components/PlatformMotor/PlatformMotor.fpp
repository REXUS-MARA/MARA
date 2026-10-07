module Mara {

    @ Absolute platform target in encoder counts
    port PlatformMoveTo(position: I32)

    @ Moves the drill platform through a Technosoft iPOS drive (CANopen CiA 402,
    @ profile position mode), reached via a Waveshare USB-CAN-A adapter on a UART.
    @ Driven only by the Orchestrator, so the platform can't move outside TEST/EXPERIMENT.
    active component PlatformMotor {

        @ Serial link to the USB-CAN adapter (connects to a byte stream driver)
        import Drv.PassiveByteStreamDriverClient

        @ Enable the drive. Also clears a drive fault.
        async input port enable: Fw.Signal drop

        @ Move to an absolute position in encoder counts, clamped to [MIN_POSITION, MAX_POSITION].
        @ 0 is the position at drive power-up.
        async input port moveTo: PlatformMoveTo drop

        @ Stop and hold position
        async input port stop: Fw.Signal drop

        @ Health ping
        async input port pingIn: Svc.Ping drop

        @ Health ping response
        output port pingOut: Svc.Ping

        # ------------------------------------------------------------------
        # Parameters
        #
        # Positions are drive encoder counts, 0 = platform position at drive
        # power-up (fully down). After commissioning, confirm which sign moves
        # the platform towards the sample: if it is negative, set MIN_POSITION
        # negative, MAX_POSITION to 0 and DRILL_POSITION negative.
        # ------------------------------------------------------------------

        @ CANopen node ID of the drive, as set in EasySetUp
        param NODE_ID: U8 default 1

        @ Lowest allowed target, in encoder counts
        param MIN_POSITION: I32 default 0

        @ Highest allowed target, in encoder counts.
        @ TODO: set after commissioning (1 mm = 4 * encoder lines / 0.794 counts).
        @ Default 0 means the platform will not move until this is set.
        param MAX_POSITION: I32 default 0

        @ Profile velocity (0x6081) for moves away from 0, i.e. the drilling feed. Technosoft internal units.
        @ 0 keeps the value stored in the drive.
        param ADVANCE_VELOCITY: U32 default 0

        @ Profile velocity (0x6081) for moves towards 0, i.e. the retract. Technosoft internal units.
        @ 0 keeps the value stored in the drive.
        param RETRACT_VELOCITY: U32 default 0

        @ Profile acceleration (0x6083), written on enable. Technosoft internal units.
        @ 0 keeps the value stored in the drive.
        param ACCELERATION: U32 default 0

        # ------------------------------------------------------------------
        # Events
        # ------------------------------------------------------------------

        @ A platform command could not be sent; the rest of that command was skipped
        event SendFailed(status: Drv.ByteStreamStatus) \
            severity warning high \
            format "Platform command not sent, CAN adapter unavailable: {}"

        @ MIN_POSITION and MAX_POSITION are both 0, so every move is clamped to 0
        event LimitsUnset \
            severity warning high \
            format "Platform limits are 0, the platform will not move"

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
