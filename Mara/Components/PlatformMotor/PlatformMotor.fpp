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

        @ Run the test cycle: enable, advance CYCLE_DISTANCE, pause, retract to 0, pause,
        @ advance CYCLE_DISTANCE again and hold there. Called by the Orchestrator like the other ports.
        async input port runTestCycle: Fw.Signal drop

        @ Rate group tick: polls the drive status word while a move runs and times the test cycle
        @ pauses. Run it at about 10 Hz, since moves are reported done on the next poll.
        async input port schedIn: Svc.Sched drop

        @ Sent once when the drive reports the commanded target reached (status word bit 10).
        @ Connect it to the Orchestrator so the next step can start.
        output port moveDone: Fw.Signal

        # Drive replies are parsed on the byte stream driver's thread and handed to the
        # component thread through these, so all state changes happen on one thread.

        @ Status word (0x6041) read back from the drive
        internal port statusWordReceived(statusWord: U16) drop

        @ CANopen emergency message from the drive
        internal port emergencyReceived(errorCode: U16, errorRegister: U8) drop

        @ The drive rejected an SDO request
        internal port sdoAbortReceived(index: U16, abortCode: U32) drop

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
        @ 4000 counts/rev and 0.795 mm/rev give 5031.4 counts per mm. 251890 = 50 mm.
        param MAX_POSITION: I32 default 251890

        @ Profile velocity (0x6081) for moves away from 0, i.e. the drilling feed. Technosoft internal units:
        @ counts per 1 ms sample * 65536. 494611 = 1.5 mm/s. 0 keeps the value stored in the drive.
        param ADVANCE_VELOCITY: U32 default 494611

        @ Profile velocity (0x6081) for moves towards 0, i.e. the retract. Same units as ADVANCE_VELOCITY.
        @ 494611 = 1.5 mm/s. 0 keeps the value stored in the drive.
        param RETRACT_VELOCITY: U32 default 494611

        @ Profile acceleration (0x6083), written on enable. Technosoft internal units:
        @ counts per (1 ms sample)^2 * 65536. 3297 = 10 mm/s^2. 0 keeps the value stored in the drive.
        param ACCELERATION: U32 default 3297

        @ Test cycle advance target, in encoder counts. 251890 = 50 mm.
        param CYCLE_DISTANCE: I32 default 251890

        @ Test cycle pause after the first advance and after the retract, in milliseconds
        param CYCLE_PAUSE_MS: U32 default 5000

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

        @ MIN_POSITION > MAX_POSITION, or 0 (fully down) is outside [MIN_POSITION, MAX_POSITION],
        @ so the retract to 0 would be clamped short of the bottom
        event LimitsInvalid(minPosition: I32, maxPosition: I32) \
            severity warning high \
            format "Platform limits invalid: MIN {} MAX {} (need MIN <= 0 <= MAX)"
        
        event CycleStarted \
            severity activity high \
            format "Platform test cycle started"

        @ Final advance reached; the platform holds there until the drive is shut off
        event CycleDone \
            severity activity high \
            format "Platform test cycle done, holding position"

        @ Test cycle cancelled by stop, by a moveTo from the Orchestrator, or by a failed send
        event CycleAborted \
            severity warning high \
            format "Platform test cycle aborted"

        @ The drive reported the commanded target reached
        event TargetReached(position: I32) \
            severity activity high \
            format "Platform reached {} counts"

        @ No target reached within twice the expected move time plus a margin
        event MoveTimedOut(position: I32) \
            severity warning high \
            format "Platform did not reach {} counts in time"

        @ The status word shows a drive fault during a move
        event DriveFault(statusWord: U16) \
            severity warning high \
            format "Platform drive fault, status word 0x{x}"

        @ Emergency message from the drive, e.g. 0x8611 following error, 0x6100 invalid setup
        event DriveEmergency(errorCode: U16, errorRegister: U8) \
            severity warning high \
            format "Platform drive emergency 0x{x}, error register 0x{x}"

        @ The drive rejected an SDO request
        event SdoAborted(index: U16, abortCode: U32) \
            severity warning high \
            format "Platform drive rejected object 0x{x}, abort code 0x{x}"


        @ NODE_ID is not a valid CANopen node ID (1 to 127); the command was not sent
        event NodeIdInvalid(nodeId: U8) \
            severity warning high \
            format "NODE_ID {} is not a valid CANopen node (1-127), command not sent"

        # ------------------------------------------------------------------
        # Telemetry
        # ------------------------------------------------------------------

        @ Last commanded target position
        telemetry TargetPosition: I32

         @ Last status word read from the drive
        telemetry StatusWord: U16 format "0x{x}"

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
