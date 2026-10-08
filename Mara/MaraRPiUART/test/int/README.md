# MARA integration tests

These tests drive the real flight software through the GDS, using the F´ GDS pytest plugin (`fprime_test_api`). The FSW and the GDS run as usual, and `pytest` connects to the GDS, sends commands, and checks events and telemetry.

Unit tests are separate. They run with `fprime-util check --all` (see the top-level README).

## Suites

| Marker | Where the FSW runs | What's attached | Run with |
|---|---|---|---|
| `software` | laptop (native build) or dev Pi | nothing | `--mara-target laptop` or `devpi` |
| `devpi` | dev Pi | nothing | `--mara-target devpi --mara-pi <host>` |
| `fakedrive` | dev Pi | `fake_drive.py` instead of the motor drive | add `--mara-fake-drive` |
| `gpio` | dev Pi | spare outputs wired to LO/SOE/EODS, plus the fake drive | add `--mara-gpio-drive 5,6,13` |
| `hardware` | flight Pi | everything; operator at the bench | `--mara-target flightpi --mara-pi <host> -s` |

Tests whose requirements aren't met are skipped, and the skip message says why. Every test ends with `exitTestMode`, which turns the drill and camera off and moves the platform to 0.

## Common arguments

Every run needs the dictionary that matches the FSW binary, for example:

```bash
DICT=build-artifacts/aarch64-linux/Mara_MaraRPiUART/dict/MaraRPiUARTTopologyDictionary.json
pytest Mara/MaraRPiUART/test/int -m <marker> --dictionary $DICT [mara options]
```

Add `--gen-junitxml --logs <dir>` to keep a JUnit report and the test log as a record.

## Laptop (no Pi)

The GDS only accepts serial ports that pyserial can list, which excludes pseudo-terminals. `pty_link.py` therefore gives the FSW a pty and the GDS a TCP port, the same way `Mara/ansible/uart_tcp_bridge.py` simulates the RXSM link.

```bash
python3 Mara/MaraRPiUART/test/int/pty_link.py &                      # /tmp/mara_fsw_uart <-> tcp 50000
build-artifacts/Darwin/Mara_MaraRPiUART/bin/Mara_MaraRPiUART -d /tmp/mara_fsw_uart -b 115200 &
fprime-gds -n --gui none --dictionary build-artifacts/Darwin/Mara_MaraRPiUART/dict/MaraRPiUARTTopologyDictionary.json \
    --communication-selection ip --ip-client --ip-address 127.0.0.1 --ip-port 50000 &
pytest Mara/MaraRPiUART/test/int -m software \
    --dictionary build-artifacts/Darwin/Mara_MaraRPiUART/dict/MaraRPiUARTTopologyDictionary.json
```

Start the FSW from a scratch directory. It writes `PrmDb.dat` to its working directory.

## Dev Pi

1. Set up ssh with a key, so `ssh <host>` works without a password; the tests use `BatchMode`. Give `<host>` passwordless `sudo`, which `fake_drive.py` needs.
2. Copy the aarch64 binary to the Pi as `~/MARA` and start it as usual. The `devpi`/`gpio` tests restart it themselves, using `--mara-fsw-dir` and `--mara-fsw-args`.
3. Start the GDS on the laptop against the Pi's UART as usual.

```bash
pytest Mara/MaraRPiUART/test/int -m "software or devpi" --dictionary $DICT --mara-target devpi --mara-pi pi@mara-dev
pytest Mara/MaraRPiUART/test/int -m fakedrive --dictionary $DICT --mara-target devpi --mara-pi pi@mara-dev --mara-fake-drive
pytest Mara/MaraRPiUART/test/int -m gpio --dictionary $DICT --mara-target devpi --mara-pi pi@mara-dev \
    --mara-fake-drive --mara-gpio-drive 5,6,13
```

**Fake drive.** `fake_drive.py` is copied to the Pi and run with `sudo`. It creates a pty, links it at the motor UART path compiled into the FSW (`MotorUartDevice` in `MaraRPiUARTTopologyDefs.hpp`, read automatically), and logs every Waveshare frame as a decoded SDO. Stopping it removes the link and closes the pty, which looks to the FSW exactly like an unplugged adapter.

**GPIO wiring for `gpio`.**
- Connect three spare GPIO outputs to the inputs: LO = BCM 27, SOE = BCM 22, EODS = BCM 17. Pass the outputs as `--mara-gpio-drive LO,SOE,EODS` in BCM numbers.
- Install `gpiod` on the Pi. The tests detect libgpiod v1 or v2 automatically.
- The FSW opens `/dev/gpiochip4`. Check that this is the right chip on the dev Pi; pass the same chip with `--mara-gpio-chip`.

## Flight Pi (operator present)

```bash
pytest Mara/MaraRPiUART/test/int -m hardware -s --dictionary $DICT --mara-target flightpi --mara-pi pi@mara-flight \
    --mara-test-position 500
```

- `--mara-test-position` is a small, safe platform move in encoder counts. Motion tests are skipped without it.
- Motion steps ask you to confirm what the platform did. PlatformMotor doesn't read the drive's replies yet, so you are the position sensor.
- `test_preflight_checklist` is the last step before integration. It moves nothing. It checks:
  - the FSW is not in TEST mode;
  - the effective flight parameters, read from `PrmDb.dat` on the Pi over the dictionary defaults: `DRILL_POSITION` ≠ 0, limits set, 0 inside `[MIN, MAX]` (otherwise the retract is clamped short of the bottom), and `DRILL_POSITION` inside the limits;
  - the adapter is present;
  - there is free space on both SD cards;
  - there are no current WARNING_HI events.
- The tests never save parameters. To save flight values, do it deliberately: `<component>.<PARAM>_PRM_SET`, then `<component>.<PARAM>_PRM_SAVE` (this stages it in PrmDb), then `FileHandling.prmDb.PRM_SAVE_FILE`.

## Sequences

`Mara/MaraRPiUART/seq/*.seq` are compiled with `fprime-seqgen` against the dictionary and run with `Mara.cmdSeq.CS_RUN <path> NO_BLOCK`. `pad_check.seq` doubles as the team's pad-check procedure.

The file path argument must be **shorter than 40 characters**. Command string arguments are limited to `FW_CMD_STRING_MAX_SIZE` = 40 at runtime, even though the dictionary advertises 240 for `CS_RUN`. A longer path is rejected with FORMAT_ERROR.
