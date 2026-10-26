module Mara {

    @ Replaces Svc.FatalHandler in CdhCore. The stock handler aborts the process on any FATAL;
    @ MARA has no defined recovery path after a reset, so this one logs a warning, counts the
    @ FATAL, and lets the software carry on.
    @
    @ What "carry on" means: F´ FW_ASSERTs report through Svc.AssertFatalAdapter, which raises the
    @ FATAL and then returns, so execution continues after the failed assert (e.g. a message that
    @ did not fit in a full queue is dropped). The adapter still aborts after FW_ASSERT_COUNT_MAX
    @ (10) asserts in the software's lifetime, as a backstop against an assert storm.
    passive component FatalHandler {

        @ FATAL announcements from the event manager. Called on the thread that logged the FATAL.
        sync input port FatalReceive: Svc.FatalEvent

        @ A FATAL occurred and was survived. Throttled so a storm cannot flood the downlink;
        @ FatalCount keeps counting.
        event FatalSurvived(eventId: FwEventIdType, count: U32) \
            severity warning high \
            format "FATAL event {} occurred ({} so far); continuing without reset" \
            throttle 20

        @ FATALs since boot
        telemetry FatalCount: U32

        @ Port for requesting the current time
        time get port timeCaller

        @ Enables event handling
        import Fw.Event

        @ Enables telemetry channels handling
        import Fw.Channel

    }
}
