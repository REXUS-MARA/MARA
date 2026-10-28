# Data products: getting the sensor data

The sensor components log every reading into F´ data products (DPs). A DP is a binary file
(`.fdp`) holding a header and a list of records. This page covers what is in the files, how to get
them off the Pi or down the radio link, and how to turn them into tables.

## What is recorded

| Component | Rate | Container | Records, in file order | One file every |
|---|---|---|---|---|
| `adxl345Manager` | 1 Hz | `AccelContainer` | `ConfigRecord` (range, rate), then 100 × `AccelRecord` (time, acceleration in g) | 100 s |
| `bmp280Manager` | 20 Hz | `ReadingContainer` | `CalibRecord` (12 trimming values), `ConfigRecord` (oversampling, filter, standby), then 500 × `ReadingRecord` (time, pressure Pa, temperature °C, raw ADC values, status) | 25 s |

Rules every sensor follows:
- Readings are stored already converted to physical units, like the telemetry.
- Each file starts with what is needed to interpret it: the sensor configuration (and, for the
  BMP280, its factory calibration). Each file can be decoded on its own.
- All readings in a file share that configuration. When a command or parameter changes it, the
  open file is closed early and the next one starts with the new configuration.
- A file is only written when it is full (or closed early); a reboot loses the readings of the
  file that was still open.

BMP280 specifics (pressure below the 300 hPa sensor range, recomputing from raw values) are in
[`Mara/Components/Bmp280Manager/docs/sdd.md`](Mara/Components/Bmp280Manager/docs/sdd.md).

## Where the files are

On the Pi, `DpCat/` in the FSW's working directory (`/home/<user>/mara/DpCat` with the ansible
setup), named `Dp_<container id>_<seconds>_<microseconds>.fdp`. lsyncd mirrors them to the second
SD card (`/mnt/sd2/dp-backup`). `DpCat/DpState.dat` records which files were already downlinked.

## Getting the files

### From the Pi or the SD cards (recovery, bench tests)

```bash
scp '<pi>:mara/DpCat/*.fdp' dp/
```

or copy `dp-backup/` from the second SD card.

### Over the downlink

Send, in this order:

```
DataProducts.dpCat.BUILD_CATALOG
DataProducts.dpCat.START_XMIT_CATALOG WAIT
```

`BUILD_CATALOG` scans `DpCat/` and lists the files not yet sent (it skips everything marked in
`DpState.dat`). `START_XMIT_CATALOG` hands them, highest priority and oldest first, to
`FileHandling.fileDownlink`, which sends them through the normal com stack. The GDS saves them in
`<--file-storage-directory>/fprime-downlink/` (default `/tmp/<user>/fprime-downlink/`).

Things to know:
- **No uplink is needed for the transfer.** The FSW writes the frames to the UART whether or not
  anyone listens; the ground station only receives. Only the two commands have to come from
  somewhere (ground, or a sequence on board).
- **The catalog is a snapshot.** Files written after `BUILD_CATALOG` are not sent until the next
  `BUILD_CATALOG` + `START_XMIT_CATALOG`. Repeat the pair to keep sending new files.
- **Each file is sent once.** File downlink has no acknowledgement or retransmission, and the file
  is marked as sent in `DpState.dat` even if the ground missed packets. Lost files stay on the Pi
  and the backup SD card. To send everything again, `DataProducts.dpCat.CLEAR_CATALOG`, delete
  `DpCat/DpState.dat`, and build again.
- `BUILD_CATALOG` is refused while a transmission is running.
- Telemetry `DataProducts.dpCat.CatalogDps` and `DataProducts.dpCat.DpsSent` show progress.

## Decoding

Use the dictionary of the build that produced the files
(`build-artifacts/aarch64-linux/Mara_MaraRPiUART/dict/MaraRPiUARTTopologyDictionary.json`, or the
`MARADict<tag>.json` release asset). With the fprime venv active:

### Everything into CSV tables (recommended)

```bash
python3 Mara/tools/dp_to_csv.py <dictionary.json> csv/ dp/
```

It decodes every `.fdp` under `dp/` and writes one CSV per record type, merged across files and
sorted by time, e.g. `csv/Mara.bmp280Manager.ReadingRecord.csv`:

```
file,container_time,time_stamp.timeBase,time_stamp.timeContext,time_stamp.seconds,time_stamp.useconds,data.pressure,data.temperature,rawPressure,rawTemperature,status,recomputed_temperature,recomputed_pressure
```

Every row keeps the file it came from, so it can be joined with that file's `ConfigRecord` and
`CalibRecord` rows. For BMP280 readings, `recomputed_pressure` is the pressure computed from the raw
values without the 300 hPa clamp; use it where `status` is 3.

### One file by hand

```bash
fprime-dp-write Dp_<...>.fdp <dictionary.json>
```

writes `Dp_<...>.json` in the current directory and checks the header and data CRCs. The JSON is a
list: the header first, then one entry per record. `dataId` identifies the record type (the
`records` section of the dictionary maps it to a name), and structs are lists of one-key objects:

```json
[ {"PacketDescriptor": 5, "Id": 268709888, "Seconds": ..., "DataSize": 16035, ...},
  {"dataId": 268709888, "data": [{"t1": 27504}, {"t2": 26435}, ...]},
  {"dataId": 268709890, "data": [{"oversampling": "STANDARD_RESOLUTION"}, {"filter": "OFF"}, {"standby": "MS_0_5"}]},
  {"dataId": 268709889, "data": [{"time_stamp": [...]}, {"data": [{"pressure": 100653.27}, {"temperature": 25.08}]},
                                 {"rawPressure": 415148}, {"rawTemperature": 519888}, {"status": 0}]},
  ... ]
```

## Adding a sensor

Follow `Bmp280Manager`: a container holding a configuration record first (plus calibration if the
sensor has one), readings in physical units with a `Fw.TimeValue` time stamp, the container closed
early when the configuration changes, and the container size checked against
`DataProductsConfig::BuffMgr::dpBufferStoreSize` (20000 bytes). `dp_to_csv.py` picks new record
types up from the dictionary without changes.
