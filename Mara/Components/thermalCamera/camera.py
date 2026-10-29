from time import sleep
import struct
import time
import sys

from seekcamera import (
    SeekCameraIOType,
    SeekCameraManager,
    SeekCameraManagerEvent,
    SeekCameraFrameFormat,
)


def on_frame(camera, camera_frame, file):
    """Async callback fired whenever a new frame is available.

    Parameters
    ----------
    camera: SeekCamera
        Reference to the camera for which the new frame is available.
    camera_frame: SeekCameraFrame
        Reference to the class encapsulating the new frame (potentially
        in multiple formats).
    file: TextIOWrapper
        User defined data passed to the callback. This can be anything
        but in this case it is a reference to the open CSV file to which
        to log data.
    """
    frame = camera_frame.thermography_float
    frametime = time.time()
    print(
        "frame available: {cid} (size: {w}x{h})".format(
            cid=camera.chipid, w=frame.width, h=frame.height
        )
    )

    print(frame.data[0][0])
    # Append the frame to the CSV file.
    for f in file:
        f.write(frame.data)
        f.write(struct.pack("d",frametime))
        f.flush()


def on_event(camera, event_type, event_status, _user_data):
    """Async callback fired whenever a camera event occurs.

    Parameters
    ----------
    camera: SeekCamera
        Reference to the camera on which an event occurred.
    event_type: SeekCameraManagerEvent
        Enumerated type indicating the type of event that occurred.
    event_status: Optional[SeekCameraError]
        Optional exception type. It will be a non-None derived instance of
        SeekCameraError if the event_type is SeekCameraManagerEvent.ERROR.
    _user_data: None
        User defined data passed to the callback. This can be anything
        but in this case it is None.
    """
    print("{}: {}".format(str(event_type), camera.chipid))
    files = sys.argv[1:]
    file = []
    if event_type == SeekCameraManagerEvent.CONNECT:
        # Open a new CSV file with the unique camera chip ID embedded.
        try:
            if [] == files:
                files = ["localthermal.dat"]
            for path in files:
                file.append(open(path, "bw"))
        except OSError as e:
            print("Failed to open file: %s" % str(e))
            return

        # Start streaming data and provide a custom callback to be called
        # every time a new frame is received.
        camera.register_frame_available_callback(on_frame, file)
        camera.capture_session_start(SeekCameraFrameFormat.THERMOGRAPHY_FLOAT)

    elif event_type == SeekCameraManagerEvent.DISCONNECT:
        camera.capture_session_stop()

    elif event_type == SeekCameraManagerEvent.ERROR:
        print("{}: {}".format(str(event_status), camera.chipid))

    elif event_type == SeekCameraManagerEvent.READY_TO_PAIR:
        return


def main():
    # Create a context structure responsible for managing all connected USB cameras.
    # Cameras with other IO types can be managed by using a bitwise or of the
    # SeekCameraIOType enum cases.
    with SeekCameraManager(SeekCameraIOType.USB) as manager:
        # Start listening for events.
        manager.register_event_callback(on_event)

        while True:
            sleep(0.1)


if __name__ == "__main__":
    main()
