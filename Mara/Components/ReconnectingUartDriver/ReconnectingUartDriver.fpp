module Mara {

    @ Serial (UART) byte stream driver that survives the device going away.
    @ Based on Drv.LinuxUartDriver, but the read task opens the device itself and keeps
    @ retrying, so the software runs without the device attached and picks it up
    @ when it is plugged in, or comes back after dropping out, without a restart.
    passive component ReconnectingUartDriver {

        @ ready, $recv, $send, recvReturnIn
        import Drv.ByteStreamDriver

        @ Allocation port used for allocating memory in the receive task
        output port allocate: Fw.BufferGet

        @ Deallocation of allocated buffers
        output port deallocate: Fw.BufferSend

        # ------------------------------------------------------------------
        # Events
        # ------------------------------------------------------------------

        @ The device could not be opened. Logged once per disconnected period.
        event AdapterNotConnected(device: string size 80) \
            severity warning high \
            format "Serial device {} not connected, retrying"

        @ The device was opened and configured
        event AdapterConnected(device: string size 80) \
            severity activity high \
            format "Serial device {} connected"

        @ The device went away while open (hangup or read error)
        event AdapterDisconnected(device: string size 80, err: I32) \
            severity warning high \
            format "Serial device {} disconnected, errno {}"

        @ No receive buffer available. Logged once until a buffer is available again.
        event NoBuffers(device: string size 80) \
            severity warning high \
            format "Serial device {}: no receive buffers"

        # ------------------------------------------------------------------
        # Telemetry
        # ------------------------------------------------------------------

        @ Total bytes sent
        telemetry BytesSent: FwSizeType

        @ Total bytes received
        telemetry BytesReceived: FwSizeType

        # ------------------------------------------------------------------
        # Special ports
        # ------------------------------------------------------------------

        @ Port for requesting the current time
        time get port timeCaller

        @ Enables event handling
        import Fw.Event

        @ Enables telemetry channels handling
        import Fw.Channel

    }
}
