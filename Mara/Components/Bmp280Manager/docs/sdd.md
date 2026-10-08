# Mara::Bmp280Manager

Reads a Bosch BMP280 pressure/temperature sensor over I2C and logs every reading into data products.
Register access and compensation use Bosch's BMP2 Sensor API, vendored unmodified in `bmp2/`
(see `bmp2/README.md` for where it came from and how it was checked).

In MaraRPiUART the sensor sits on `/dev/i2c-1` (shared with the ADXL345) at address 0x77 and is
ticked by rateGroup2 at 20 Hz.

## Design

Queued component (no thread of its own). `run` queues a `tick` signal and then dispatches the queue
on the rate group thread. The shared `Mara.I2CSensorStateMachine` (`Components/Common`) drives the
sensor, one state per tick:

| State | Action | What it does |
|---|---|---|
| RESET | `doReset` | Reads `I2C_ADDRESS`, `bmp2_soft_reset()` (0xB6 to 0xE0, 2 ms delay) |
| WAIT_RESET | `checkReset` | `bmp2_get_status()`: waits until `im_update` is 0 (NVM trimming copied) |
| ENABLE | `doEnable` | `bmp2_init()`: chip ID must be 0x58, reads the trimming parameters |
| CONFIGURE | `doConfigure` | `bmp2_set_power_mode(NORMAL)` with the oversampling/filter/standby parameters |
| RUN | `doRead` | Burst-reads 0xF7..0xFC, compensates with `bmp2_compensate_data()` |

The first reading arrives on the 5th tick. A bus error in any state sends `error`, which goes back
to RESET; recovery after a disconnect is automatic.

In normal mode the sensor measures continuously and the data registers are shadowed, so `doRead`
never waits on the `measuring` status bit. The fprime-sensors BmpManager did wait on it, and it also
shifted the oversampling values twice, which set pressure to SKIP. Those two bugs are why its data
never updated.

### Out-of-range readings (flight)

The BMP280 is specified for 300..1100 hPa and -40..85 °C. Bosch's compensation clamps to those
limits and returns a warning (`BMP2_W_MIN_PRES` = 3 etc.). Above roughly 9 km every compensated
pressure therefore reads 30000 Pa. The raw 20-bit ADC values stay valid, so every record keeps them
next to the compensated values and its Bosch `status`, and every container starts with the
calibration and the sensor configuration. Pressure outside the specified range can be recomputed on the ground (outside the
datasheet accuracy) with the datasheet double-precision formula, without the clamp:

```python
def compensate(c, adc_t, adc_p):          # c: CalibRecord fields t1..t3, p1..p9
    v1 = (adc_t / 16384.0 - c.t1 / 1024.0) * c.t2
    v2 = (adc_t / 131072.0 - c.t1 / 8192.0) ** 2 * c.t3
    t_fine = v1 + v2
    v1 = t_fine / 2.0 - 64000.0
    v2 = v1 * v1 * c.p6 / 32768.0 + v1 * c.p5 * 2.0
    v2 = v2 / 4.0 + c.p4 * 65536.0
    v1 = (c.p3 * v1 * v1 / 524288.0 + c.p2 * v1) / 524288.0
    v1 = (1.0 + v1 / 32768.0) * c.p1
    p = (1048576.0 - adc_p - v2 / 4096.0) * 6250.0 / v1
    p += (c.p9 * p * p / 2147483648.0 + p * c.p8 / 32768.0 + c.p7) / 16.0
    return t_fine / 5120.0, p            # °C, Pa
```

If the temperature is clamped, Bosch does not compute the pressure (it stays 0); the raw values
still allow both to be recomputed.

## Port Descriptions
| Name | Description |
|---|---|
| run | Svc.Sched tick from the rate group |
| busWriteRead | I2C register reads (`Drv.I2cWriteRead`) |
| busWrite | I2C register writes (`Drv.I2c`) |
| productGetOut / productSendOut | Data product container allocation and sending |

## Parameters
| Name | Default | Description |
|---|---|---|
| I2C_ADDRESS | 0x77 | Device address (0x76 with SDO low). Changing it resets the sensor |
| OVERSAMPLING | STANDARD_RESOLUTION | Bosch preset: pressure x4, temperature x1, 11.5 ms per measurement |
| FILTER | OFF | IIR filter coefficient (a filter adds lag during ascent) |
| STANDBY | MS_0_5 | Standby between measurements in normal mode |

Changing OVERSAMPLING, FILTER or STANDBY reconfigures the sensor on the next tick.
The measurement time plus standby must stay below the 50 ms tick for every reading to be fresh.

## Commands
| Name | Description |
|---|---|
| RESET | Reset the sensor and run the start-up sequence again |

## Events
| Name | Severity | Description |
|---|---|---|
| I2cError | warning high | I2C transfer failed (address, driver status); the sensor is reset |
| ChipIdMismatch | warning high | Chip ID is not 0x58 (wrong device on the address) |
| BoschError | warning high | Other Bosch API failure |
| BadReading | warning low | Raw value outside the ADC range; the record is still stored |
| OutOfSpec | warning low | Reading clamped to the sensor range; raw values are in the data product |
| Configured | activity high | Sensor configured; measurement + standby time in µs |
| DpMemoryFailure | warning high | No data product buffer available |

## Telemetry
| Name | Description |
|---|---|
| Reading | Last compensated pressure (Pa) and temperature (°C) |
| Status | Bosch result of the last reading: 0 OK, 1..4 clamped, <0 error |

## Data Products
Container `ReadingContainer`: one `CalibRecord` (Bmp280Calib, the 12 trimming values), one
`ConfigRecord` (Bmp280Config: oversampling, filter, standby), then `RECORD_COUNT` = 500
`ReadingRecord`s (Bmp280DataTimed: time, pressure, temperature, raw ADC values, status). That is
one container every 25 s at 20 Hz; 16100 bytes per packet, below the 20000-byte DataProducts buffer
(a `static_assert` guards this).

All readings in a container use the configuration in its `ConfigRecord`: when a parameter change
reconfigures the sensor, the open container is sent early and the next one starts with the new
configuration. A reset that reconfigures with the same settings (e.g. after a bus error) keeps the
container open.

`fprime-dp-write <file>.fdp <dictionary>.json` decodes a container into a JSON list: the header,
then one `{"dataId": ..., "data": {...}}` entry per record. The record type follows from `dataId`
(component base ID + 0 calibration, + 1 reading, + 2 configuration). To rebuild the flight
timeline, decode all files, sort them by header time and concatenate the reading records; only
readings with `status` 3 (`BMP2_W_MIN_PRES`) need the recompute above.

## Unit Tests
`test/ut` simulates the sensor's register map with the datasheet section 3.12 example calibration.

| Name | Description |
|---|---|
| Startup.Sequence | RESET → WAIT_RESET → ENABLE → CONFIGURE → RUN, first reading on tick 5 |
| Startup.WaitsForNvmCopy | Stays in WAIT_RESET while `im_update` is set |
| Startup.WrongChipId | ChipIdMismatch and back to RESET |
| Reading.DatasheetCompensation | adc_T 519888, adc_P 415148 → 25.08 °C, 100653.27 Pa |
| Reading.LowPressureClamp | 181 hPa reads as 30000 Pa with OutOfSpec; raw value kept in the data product |
| Config.Registers | ctrl_meas = 0x2F, config = 0x00 with the defaults |
| Config.ReconfigureOnParameter | Parameter changes rewrite ctrl_meas/config |
| Recovery.BusError | Bus failure resets the sensor, readings resume after the bus is back |
| Recovery.ResetCommand | RESET command restarts the sequence |
| DataProduct.FullContainer | Calibration and configuration first, 500 readings, then a new container |
| DataProduct.ReconfigureClosesContainer | New settings send the open container early; a same-settings reset does not |
