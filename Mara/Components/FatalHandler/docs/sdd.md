# Mara::FatalHandler

Replaces `Svc.FatalHandler` in the CdhCore subtopology, through the F´ config override in `Mara/MaraRPiUART/FppConfigOverrides/CdhCoreFatalHandlerConfigOverride/`.

The stock Linux handler waits one second and then aborts the process on any FATAL. MARA has no defined recovery path after a reset, so this handler logs the FATAL as a warning, counts it, and returns. The software keeps running.

## Behaviour
| On | Does |
|---|---|
| `FatalReceive(id)` | Increments the FATAL count, logs `FatalSurvived(id, count)` (WARNING_HI) and writes `FatalCount` telemetry. Never aborts or exits. |

- `FatalSurvived` is throttled to 20 events, so a FATAL storm can't flood the downlink. `FatalCount` keeps counting.
- The port is called on whichever thread logged the FATAL, so the count is atomic.

## What "keeps running" means
- **After a failed `FW_ASSERT`**, `Svc.AssertFatalAdapter` logs the FATAL and then returns, so execution continues *after the failed assert*. Usually the failed operation is simply skipped; for example, a message that didn't fit in a full queue is dropped. An assert that guarded memory safety, such as an index check, gives no such guarantee.
- **The assert-storm backstop stays.** `Svc.AssertFatalAdapter` still aborts after `FW_ASSERT_COUNT_MAX` (10) asserts in the software's lifetime. That's an F´ framework constant (`FpConstants.fpp`).
- **FATAL events logged directly by a component** (`log_FATAL_*`) are handled the same way.

## Verified
- Unit tests: a FATAL is survived, logged and counted; warnings stop at the throttle while the count continues.
- End to end: the ThreadSanitizer build of the deployment used to abort when a 2000-command burst overflowed `CdhCore.cmdDisp`'s queue. With this handler it logged `FatalSurvived` and kept running.
