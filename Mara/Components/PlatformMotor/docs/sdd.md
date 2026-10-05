# Mara::PlatformMotor

Moves the drill platform. The drive is a Technosoft iPOS (closed-loop stepper) using CANopen CiA 402 in profile position mode. It is reached through a Waveshare USB-CAN-A adapter, which appears as a CH341 serial port at 2 Mbps and is driven by `Drv.LinuxUartDriver` (`motorUart`).

The component has no commands of its own. Only the Orchestrator drives it, so the platform can move only in TEST (from ground commands) or during the EXPERIMENT timeline.

## Ports
| Name | Type | Description |
|---|---|---|
| `enable` | async `Fw.Signal` | Sets mode 1, writes `ACCELERATION` if it is non-zero, then fault reset → shutdown → switch on → enable operation |
| `moveTo` | async `PlatformMoveTo(position: I32)` | Absolute move in encoder counts, clamped to `[MIN_POSITION, MAX_POSITION]` |
| `stop` | async `Fw.Signal` | Halt (controlword 0x010F); the drive holds position |
| `toByteStreamDriver`, `fromByteStreamDriver`, `fromByteStreamDriverReturn`, `byteStreamDriverReady` | `Drv.PassiveByteStreamDriverClient` | Link to `motorUart` |
| `pingIn` / `pingOut` | `Svc.Ping` | Health |

## Parameters
| Name | Default | Description |
|---|---|---|
| `NODE_ID` | 1 | CANopen node ID set in EasySetUp |
| `MIN_POSITION` | 0 | Lowest allowed target (counts) |
| `MAX_POSITION` | 0 | Highest allowed target (counts). **Must be set after commissioning.** At 0 the platform never moves. 1 mm = 4 × encoder lines / 0.794 counts. |
| `ADVANCE_VELOCITY` | 0 | Profile velocity (0x6081) for upward moves (the drilling feed). 0 = keep the drive's stored value |
| `RETRACT_VELOCITY` | 0 | Profile velocity for downward moves. 0 = keep the drive's stored value |
| `ACCELERATION` | 0 | Profile acceleration (0x6083), written on enable. 0 = keep the drive's stored value |

The direction of a move (and therefore which velocity is used) is decided by comparing the new target with the last commanded one.

## Behaviour
- Writes are fire-and-forget expedited SDOs with a 10 ms gap between them. Replies are not parsed yet; received buffers are returned straight away.
- A move writes controlword 0x0F (so the next setpoint is a rising edge), then the target (0x607A), then 0x3F (new setpoint, change immediately). A retract therefore overrides an unfinished advance.
- Position 0 is wherever the platform was at drive power-up. Homing is not implemented yet.
- There are no FATAL events. A send failure raises `SendFailed` (WARNING_HI).

## Not done yet
- Parsing drive replies: SDO aborts as events, statusword and actual position as telemetry.
- Homing.
- The real `/dev/serial/by-id/` path, which is `MotorUartDevice` in `MaraRPiUARTTopologyDefs.hpp`.
