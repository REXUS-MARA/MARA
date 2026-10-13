# Bosch BMP2 Sensor API (vendored)

Unmodified copy of Bosch Sensortec's BMP2 Sensor API **v1.0.2** (2023-04-28), BSD-3-Clause (see `LICENSE`).
Used by `Bmp280Manager` for register access and compensation. Do not edit or reformat these files.

## Source

Bosch removed the official repositories (`github.com/boschsensortec/BMP2_SensorAPI`,
`github.com/boschsensortec/BMP280_driver`) when the BMP280 was discontinued. These files come from
the mirror `github.com/ByteTheFox91/BMP2_SensorAPI` (`src/bmp2.c`, `include/bmp2.h`,
`include/bmp2_defs.h`, `LICENSE`).

## Verification (2026-10-08)

Diffed against an independent copy of v1.0.1 (`github.com/ahmadasmandar/BMP2-Sensor-API`, 2021).
The only differences are the v1.0.2 release changes: copyright/version headers, a 2 ms delay after
the soft reset in `bmp2_soft_reset()`, and `const void *intf_ptr` in the read/write callback typedefs.
Compensation, parsing and register code are identical.
