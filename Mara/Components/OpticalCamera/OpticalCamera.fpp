module Mara {
    @ Component for recording the camera
    active component OpticalCamera {

        @ Turns on the camera recording
        async input port Camera_ON: Fw.Signal

        @ Turns off the camera recording
        async input port Camera_OFF: Fw.Signal

        @ Health ping
        async input port pingIn: Svc.Ping

        @ Health ping response
        output port pingOut: Svc.Ping

        @ Enables command handling
        import Fw.Command
        
        @ Port for requesting the current time
        time get port timeCaller

        @ Enables event handling
        import Fw.Event

        @ Enables telemetry channels handling
        import Fw.Channel

        @ Port to return the value of a parameter
        param get port prmGetOut

        @Port to set the value of a parameter
        param set port prmSetOut

        @ Camera device node
        param DEVICE: string size 128 default "/dev/v4l/by-id/usb-XXXX-video-index0"

        @ Capture resolution, as ffmpeg expects it
        param VIDEO_SIZE: string size 16 default "1920x1080"

        @ Capture framerate
        param FRAMERATE: U32 default 30

        @ Output directory on SD card 1
        param PRIMARY_DIR: string size 128 default "/mnt/sd1/camera"

        @ Output directory on SD card 2
        param BACKUP_DIR: string size 128 default "/mnt/sd2/camera"

        @ ffmpeg stops by itself after this many seconds
        param MAX_SECONDS: U32 default 900

        # ------------------------------------------------------------------
        # Events
        # ------------------------------------------------------------------

        event RecordingStarted(seg: U32) \
        severity activity high \
        format "Recording segment {} started"

        event RecordingStopped(seg: U32) \
        severity activity high \
        format "Recording segment {} stopped"

        @ fork() failed
        event SpawnFailed(err: I32) \
        severity warning high \
        format "Failed to start ffmpeg, errno {}"

        @ ffmpeg exited on its own with an error
        event RecorderExitedEarly(seg: U32, status: I32) \
        severity warning high \
        format "ffmpeg for segment {} exited unexpectedly, raw status {}"

        @ Output file stopped growing while ffmpeg is still running
        event RecorderStalled(seg: U32, bytes: U64) \
        severity warning high \
        format "Segment {} stalled at {} bytes"


    }
}