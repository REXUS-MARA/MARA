# Mara::OpticalCamera

Component for recording the camera.

## Starting and stopping ffmpeg
- **Start (`Camera_ON`):** `posix_spawnp` starts ffmpeg with **default SIGINT/SIGTERM handling and no blocked signals**. Without that, an FSW started from a background shell (e.g. `nohup ./MARA ... &`), where SIGINT is ignored, would pass the ignore on to ffmpeg, and OFF could never stop it.
- **Stop (`Camera_OFF`, and the destructor):** sends SIGINT so ffmpeg finalises both files, then waits up to `STOP_TIMEOUT_SECONDS` (default 5).
  - If ffmpeg hasn't exited by then (for example the camera wedged), it is killed: `RecorderKilled`, WARNING_HI. The end of that segment may not be finalised.
  - If even SIGKILL doesn't reap it within 1 s (stuck in the kernel), it is abandoned: `RecorderUnresponsive`, WARNING_HI. A new ON starts a new process.
  - OFF never blocks indefinitely, so the component keeps answering health pings.
  - The destructor stops the recorder the same way but logs nothing, because at teardown the event components may already be gone.

## Usage Examples
Add usage examples here

### Diagrams
Add diagrams here

### Typical Usage
And the typical usage of the component here

## Class Diagram
Add a class diagram here

## Port Descriptions
| Name | Description |
|---|---|
|---|---|

## Component States
Add component states in the chart below
| Name | Description |
|---|---|
|---|---|

## Sequence Diagrams
Add sequence diagrams here

## Parameters
| Name | Description |
|---|---|
|---|---|

## Commands
| Name | Description |
|---|---|
|---|---|

## Events
| Name | Description |
|---|---|
|---|---|

## Telemetry
| Name | Description |
|---|---|
|---|---|

## Unit Tests
Add unit test descriptions in the chart below
| Name | Description | Output | Coverage |
|---|---|---|---|
|---|---|---|---|

## Requirements
Add requirements in the chart below
| Name | Description | Validation |
|---|---|---|
|---|---|---|

## Change Log
| Date | Description |
|---|---|
|---| Initial Draft |
## Queue overflow
Every async port, command and state-machine signal uses the `drop` queue-full policy instead of the F´ default, which asserts (FATAL). A dropped message is silent apart from the internal dropped-message counter, and a dropped command gets no response. With a queue depth of 10 and a few messages per minute this is theoretical, but a drop is recoverable and a FATAL is not.
