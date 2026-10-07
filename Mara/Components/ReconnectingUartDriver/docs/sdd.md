# Mara::ReconnectingUartDriver

Serial byte stream driver (`Drv.ByteStreamDriver` interface) that survives the device going away. It is based on `Drv.LinuxUartDriver`, but it is built for a USB adapter that may be missing at boot, unplugged on the bench, or drop out under vibration:

- The software starts and runs without the device.
- It warns once when the device is missing, and once when it disappears.
- It picks the device up again on its own, with no restart, when it is plugged in.

It is used for `motorUart`, the Waveshare USB-CAN-A link to the platform motor drive.

## Why not `Drv.LinuxUartDriver`
- Its `open()` is one-shot. If the device is missing at boot, it never connects.
- Its read thread can't recover after an unplug. A hung-up tty makes `read()` return 0 forever, which that driver treats as a timeout, so it spins. A `-1` read error instead logs a WARNING_HI on every loop.

## Usage
```cpp
// configureTopology(): store settings only, nothing is opened here
motorUart.configure("/dev/serial/by-id/...", Mara::ReconnectingUartDriver::BAUD_2000K,
                    Mara::ReconnectingUartDriver::NO_FLOW, Mara::ReconnectingUartDriver::PARITY_NONE, 64);
// setupTopology(): start the read task, which owns connecting
motorUart.start(priority, stackSize);
// teardownTopology()
motorUart.quitReadThread();
motorUart.join();
```
Use a `/dev/serial/by-id/` path, so a replugged adapter resolves to the same name even if it comes back as a different `ttyUSB*`.

## Behaviour
The read task loops:
1. **Not connected:** try to open and configure the device. On failure, log `AdapterNotConnected` once per disconnected period, wait 1 s, and retry. On success, log `AdapterConnected` and invoke `ready`.
2. **Connected:** `poll()` for up to 1 s.
   - A timeout just loops, which is how `quitReadThread()` is noticed.
   - A hangup or error (`POLLHUP`/`POLLERR`/`POLLNVAL`, or a read error or EOF) logs `AdapterDisconnected` once, closes the device and goes back to step 1.
   - Data: allocate a buffer, read, and send it out on `$recv`. If no buffer is available, log `NoBuffers` once and retry every 50 ms.

`$send` writes under the fd lock. When not connected, it returns `OTHER_ERROR` without logging, because the caller reports it (PlatformMotor raises `SendFailed`). A write error also returns `OTHER_ERROR` and does not close the device: a dead device fails the read task's `poll()` too, so reconnection is handled in one place.

Only the read task opens and closes the device.

## Events
| Name | Severity | When |
|---|---|---|
| `AdapterNotConnected(device)` | WARNING_HI | First failed open of a disconnected period, including at boot |
| `AdapterConnected(device)` | ACTIVITY_HI | Device opened |
| `AdapterDisconnected(device, err)` | WARNING_HI | Device went away; `err` is errno, or 0 for a hangup or EOF |
| `NoBuffers(device)` | WARNING_HI | No receive buffer; once until one is available again |

## Telemetry
`BytesSent`, `BytesReceived`: running totals, written when they change.

## Limits
- Baud rates above 230400 only exist on Linux.
- No FATAL paths. An unsupported baud rate just fails to open, and the driver keeps retrying.
