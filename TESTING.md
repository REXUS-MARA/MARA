# Testing MARA

There are three layers of tests, from fast and hardware-free to slow and hands-on:

| Layer | What | Where it runs | How long |
|---|---|---|---|
| **Unit tests** | Each component alone, with F´'s generated Tester and GTest. Run under AddressSanitizer, UBSan and (on Linux) the leak checker; optionally ThreadSanitizer. Includes random (STest) testing of the Orchestrator. | laptop, CI | ~1 min |
| **Integration tests** | The real flight software, driven through the GDS with pytest (`fprime_test_api`). | laptop, dev Pi, flight Pi | 2 min to 1 h |
| **Hardware-in-the-loop** | The same integration tests, with the motor drive, camera, accelerometer and GPIO lines attached; an operator confirms what moves. | dev Pi, flight Pi | ~20 min |

Before anything: `source fprime-venv/bin/activate`.

---

## 1. Unit tests

```bash
fprime-util generate --ut          # once, or after adding modules / CMake changes
fprime-util check --all            # build and run every component's tests
```

To run one component, `cd` into its directory (e.g. `Mara/Components/PlatformMotor`) and run `fprime-util check`.

| Component | Tests | What they cover |
|---|---|---|
| FatalHandler | 2 | FATALs are logged and survived, throttled but counted |
| Orchestrator | 15 | Flight path, exact phase timing, the retract guarantee (EODS/error while drilling), TEST mode rules (ESA: LO only logged), command gating, queue overflow, plus 18 000 random steps checked against safety invariants |
| PlatformMotor | 24 | Byte-exact CAN frames, clamping, velocity choice for both sign conventions, send failures, queue overflow; negative tests for inverted limits, 0 outside the limits, invalid NODE_ID and extreme targets |
| ReconnectingUartDriver | 16 (21 on Linux) | Real pseudo-terminals: hot-plug cycles, no event floods, buffer ownership, every baud/parity/flow setting, full output buffer |
| GPIOWatcher | 11 | Debounce, spikes, re-arming, failed reads |
| OpticalCamera | 9 | A fake `ffmpeg` on `PATH`: arguments, SIGINT on stop, segments, early exit, stalls, missing ffmpeg |

### Sanitizers (memory errors, leaks, undefined behaviour, data races)

The unit-test build is compiled with `-fsanitize=address,undefined`, and on Linux leak checking is included. A memory error, a leak, or undefined behaviour makes the test fail even if every assertion passed.

Data races need ThreadSanitizer, which can't be combined with AddressSanitizer, so it gets its own build directory:

```bash
fprime-util generate --ut --build-cache build-fprime-tsan-ut \
    -DENABLE_SANITIZER_THREAD=ON -DENABLE_SANITIZER_ADDRESS=OFF \
    -DENABLE_SANITIZER_LEAK=OFF -DENABLE_SANITIZER_UNDEFINED_BEHAVIOR=OFF
fprime-util check --all --build-cache build-fprime-tsan-ut
```

### Coverage

```bash
cd Mara/Components/Orchestrator && fprime-util check --coverage
```

This prints line, function and branch coverage and writes an HTML report to `coverage/` (gitignored). Current line coverage: 100% for every component except ReconnectingUartDriver, which is at 92.5%; the rest is error handling that can't be triggered from a test. Branch numbers look low because they include the hidden branches inside F´'s macros.

### Random tests: replaying a failure

The Orchestrator's random test prints `[STest::Random] Generated seed N`, and the seed is also appended to `seed-history` in the directory you ran from. To replay a failing run exactly, put that number in a file named `seed` in the same directory and rerun. Both files are gitignored.

### On Linux

CI runs the unit tests in the `fprime-arm:4.1.1` image. To reproduce that locally on a Mac:

```bash
container run --arch amd64 --rm -u "`id -u`:`id -g`" -v "$(pwd):/project" ghcr.io/filipjsowa/fprime-arm:4.1.1 \
    -c "fprime-util generate --ut --build-cache build-fprime-linux-ut && fprime-util check --all --build-cache build-fprime-linux-ut"
```

The image is amd64, so on an Apple-silicon Mac every compile is emulated and slow.
- **Keep the build cache in the repo** (`build-fprime-linux-ut`, gitignored), not inside the throwaway container, so later runs only rebuild what changed.
- **Skip the `generate` step** on later runs unless you added modules or changed CMake files.
- **Use a separate cache from macOS:** it must not be the macOS `build-fprime-automatic-native-ut`.

---

## 2. Integration tests

Full details are in [`Mara/MaraRPiUART/test/int/README.md`](Mara/MaraRPiUART/test/int/README.md). Here is the short version.

> **Status:** the `software` and `stability` suites have been run on a laptop against the native FSW. The `devpi`, `fakedrive`, `gpio` and `hardware` suites are written and reviewed but **have not been run yet**, because no Pi was available. Expect to fix small things (paths, timing tolerances, pin numbers) on their first run.

The tests talk to a **running GDS**. Start the FSW and the GDS first, then run pytest with the same dictionary. Each test file is one suite, selected with `-m`:

| Suite | Needs | Command (after the stack is up) |
|---|---|---|
| `software` | any FSW, no hardware | `pytest Mara/MaraRPiUART/test/int -m software --dictionary $DICT` |
| `devpi` | FSW on the dev Pi, nothing attached | add `--mara-target devpi --mara-pi pi@mara-dev` |
| `fakedrive` | dev Pi + simulated drive | add `--mara-fake-drive` |
| `gpio` | dev Pi, jumpers on LO/SOE/EODS | add `--mara-gpio-drive 5,6,13` |
| `stability` | any FSW, 30 min | `pytest ... -m stability --mara-soak-minutes 30` |
| `hardware` | flight Pi, everything attached, operator | see below |

`$DICT` is the dictionary of the binary under test. For the Pi that's `build-artifacts/aarch64-linux/Mara_MaraRPiUART/dict/MaraRPiUARTTopologyDictionary.json`; for a laptop run it's the `Darwin` one.

A test whose requirements aren't met is **skipped with the reason**. Run with `-rs` to see why.

### On a laptop (no hardware at all)

```bash
python3 Mara/MaraRPiUART/test/int/pty_link.py &      # virtual ground link: pty for the FSW, TCP for the GDS
build-artifacts/Darwin/Mara_MaraRPiUART/bin/Mara_MaraRPiUART -d /tmp/mara_fsw_uart -b 115200 &
fprime-gds -n --gui none --dictionary $DICT --communication-selection ip --ip-client --ip-address 127.0.0.1 --ip-port 50000 &
pytest Mara/MaraRPiUART/test/int -m software --dictionary $DICT
```

Start the FSW from a scratch directory; it writes `PrmDb.dat` where it runs.

---

## 3. Hardware tests

These tests move the real platform. **Read this whole section before running them.**

### Before you start

- [ ] Someone is at the bench for the whole run and can cut motor power.
- [ ] The platform's travel is clear: no sample, no hands, no cables.
- [ ] `MIN_POSITION` / `MAX_POSITION` are set for this platform (the tests refuse to move if both are 0), and **0 is inside them**.
- [ ] You know a small, safe test move in encoder counts, for `--mara-test-position`.
- [ ] The drive was commissioned in EasySetUp and jogs from there. If it doesn't move in EasySetUp, it won't move from the FSW.
- [ ] Nothing in the tests saves parameters. Don't run `PRM_SAVE_FILE` during testing unless you mean to change the flight values.

### Dev Pi: GPIO jumpers for the flight-timeline rehearsal

Wire three spare GPIO outputs to the service-module inputs: **LO = BCM 27, SOE = BCM 22, EODS = BCM 17**. Then `--mara-gpio-drive` takes the three outputs in that order, e.g. `5,6,13`. Install `gpiod` on the Pi. The FSW opens `/dev/gpiochip4`; check that this is the header chip on your Pi (`gpiodetect`) and pass the same chip with `--mara-gpio-chip`.

The rehearsal plays the service module (LO, then SOE, optionally EODS) with short phase times. It checks the timeline's events and timing, and the exact commands the platform was sent, using the fake drive. Each test restarts the FSW, because restarting is the only way back to IDLE after LO.

### Flight Pi

```bash
pytest Mara/MaraRPiUART/test/int -m hardware -s -rs --dictionary $DICT \
    --mara-target flightpi --mara-pi pi@mara-flight --mara-test-position 500 \
    --gen-junitxml --logs test-records/$(date +%F)
```

- `-s` is required: the tests ask you questions in the terminal.
- **Every motion asks you to confirm** what happened ("Did the platform move towards the sample by the expected amount?"). Answer `n` if anything is off; the test fails and the platform is sent back to 0. PlatformMotor doesn't read the drive's replies yet, so you are the position sensor.
- Every test ends by leaving TEST mode: drill off, camera off, platform to 0.
- The hot-plug test asks you to unplug and replug the USB-CAN adapter.
- Keep the `test-records/` output (JUnit XML and logs) as the test record.

### Pre-flight checklist (last step before integration)

```bash
pytest Mara/MaraRPiUART/test/int -m hardware -s -k preflight --dictionary $DICT --mara-target flightpi --mara-pi pi@mara-flight
```

This moves nothing. It checks:
- the FSW is **not in TEST** (ESA: LO is ignored in TEST, so the experiment would not run);
- the effective flight parameters, read from `PrmDb.dat` on top of the dictionary defaults: `DRILL_POSITION` ≠ 0, limits set, 0 inside `[MIN, MAX]`, `DRILL_POSITION` inside the limits, and all phase times > 0;
- the USB-CAN adapter is present;
- there are at least 4 GB free on both SD cards;
- there are no current WARNING_HI events.

### Saving flight parameters

`PRM_SET` changes the value in RAM only. To make it survive a reboot:

1. `<component>.<PARAM>_PRM_SET <value>`
2. `<component>.<PARAM>_PRM_SAVE` (stages the value in PrmDb)
3. `FileHandling.prmDb.PRM_SAVE_FILE` (writes `PrmDb.dat`)

Then run the pre-flight checklist.

---

## Troubleshooting

| Symptom | Cause |
|---|---|
| A command with a file path fails with `FORMAT_ERROR` | Command string arguments are limited to 40 characters at runtime (`FW_CMD_STRING_MAX_SIZE`), although the dictionary says more. Use short paths, e.g. `/tmp/mara_pad_check.bin`. |
| `fprime-gds --uart-device /tmp/...` says "not valid" | The GDS only accepts serial ports pyserial lists. Use `pty_link.py` and the GDS's `--ip-client` mode. |
| Unit tests pass on macOS but fail on Linux with `LeakSanitizer` | A Tester didn't call `deinit()` on an active component. |
| A missing ffmpeg shows `RecorderExitedEarly` (status 127) instead of `SpawnFailed` | glibc's `posix_spawnp` succeeds even when the program doesn't exist; the child exits 127. It's still reported, just under that event. |
| `PrmDb.dat` is empty after `PRM_SAVE_FILE` | The values were set but not staged: run `<PARAM>_PRM_SAVE` first. |
| GPIO tests can't open lines | Wrong `--mara-gpio-chip`; check with `gpiodetect`. |
