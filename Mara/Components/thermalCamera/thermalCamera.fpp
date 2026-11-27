module Mara {
    @ Thermal camera component
    active component thermalCamera {

        @ command to start camera
        async command start()

        @ command to start camera
        async command stop()

        @ command to check filesize
        async command check()

        @ control that thang from orc aswell
        async input port cameraOn: Fw.Signal
        async input port cameraOff: Fw.Signal
        async input port checkCamera: Fw.Signal

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

        event RecordingStarted() \
        severity activity high \
        format "Thermal camera started"

        event RecordingStopped() \
        severity activity high \
        format "Thermal camera stopped"

        event ReportFileSize(fileSize: U32) \
        severity activity high \
        format "Thermal camera filesize at {} KB"

    }
}
