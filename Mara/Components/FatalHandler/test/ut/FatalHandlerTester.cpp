// ======================================================================
// \title  FatalHandlerTester.cpp
// \author fsowa
// \brief  cpp file for FatalHandler component test harness implementation class
// ======================================================================

#include "FatalHandlerTester.hpp"

namespace Mara {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

FatalHandlerTester ::FatalHandlerTester()
    : FatalHandlerGTestBase("FatalHandlerTester", FatalHandlerTester::MAX_HISTORY_SIZE), component("FatalHandler") {
    this->initComponents();
    this->connectPorts();
}

FatalHandlerTester ::~FatalHandlerTester() {}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void FatalHandlerTester ::testFatalIsSurvived() {
    // Returning from the port at all is the point: the stock handler would abort here
    this->invoke_to_FatalReceive(0, 0x1234);
    this->invoke_to_FatalReceive(0, 0x5678);

    ASSERT_EVENTS_FatalSurvived_SIZE(2);
    ASSERT_EVENTS_FatalSurvived(0, 0x1234, 1);
    ASSERT_EVENTS_FatalSurvived(1, 0x5678, 2);
    ASSERT_TLM_FatalCount_SIZE(2);
    ASSERT_TLM_FatalCount(1, 2);
}

void FatalHandlerTester ::testThrottledButCounted() {
    for (U32 i = 0; i < 30; i++) {
        this->invoke_to_FatalReceive(0, i);
    }
    ASSERT_EVENTS_FatalSurvived_SIZE(20);
    ASSERT_TLM_FatalCount(this->tlmHistory_FatalCount->size() - 1, 30);
}

}  // namespace Mara
