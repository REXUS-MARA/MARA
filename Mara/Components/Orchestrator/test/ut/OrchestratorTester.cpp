// ======================================================================
// \title  OrchestratorTester.cpp
// \author fsowa
// \brief  cpp file for Orchestrator component test harness implementation class
// ======================================================================

#include "OrchestratorTester.hpp"

namespace Mara {

namespace {
const OrchestratorTester::Command TEST_COMMANDS[] = {
    OrchestratorTester::Command::DRILL_ON,         OrchestratorTester::Command::DRILL_OFF,
    OrchestratorTester::Command::PLATFORM_MOVE_TO, OrchestratorTester::Command::PLATFORM_STOP,
    OrchestratorTester::Command::CAMERA_ON,        OrchestratorTester::Command::CAMERA_OFF,
    OrchestratorTester::Command::THERMAL_ON,       OrchestratorTester::Command::THERMAL_OFF,
};
}  // namespace

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

OrchestratorTester ::OrchestratorTester()
    : OrchestratorGTestBase("OrchestratorTester", OrchestratorTester::MAX_HISTORY_SIZE), component("Orchestrator") {
    this->initComponents();
    this->connectPorts();
}

OrchestratorTester ::~OrchestratorTester() {
    // Free the component's message queue (allocated in init)
    static_cast<OrchestratorComponentBase&>(this->component).deinit();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void OrchestratorTester ::testFlightPath() {
    this->setTimeline(2, 3, 2, 500);
    ASSERT_EQ(this->state(), State::IDLE);

    this->lo();
    ASSERT_EQ(this->state(), State::FLIGHT);
    ASSERT_EVENTS_EnterFlight_SIZE(1);
    ASSERT_EQ(this->hardwareOutputs(), 0U);

    this->soe();
    ASSERT_EQ(this->state(), State::EXPERIMENT_DRILLING_SPIN_UP);
    ASSERT_EVENTS_EnterExperiment_SIZE(1);
    ASSERT_EVENTS_PhaseSpinUp_SIZE(1);
    ASSERT_EVENTS_PhaseSpinUp(0, 2);
    ASSERT_EVENTS_DrillPositionUnset_SIZE(0);
    ASSERT_from_OpticalCameraON_SIZE(1);
    ASSERT_from_PlatformEnable_SIZE(1);
    ASSERT_from_DrillON_SIZE(1);
    ASSERT_from_PlatformMoveTo_SIZE(0);  // the platform waits at the bottom during spin-up
}

void OrchestratorTester ::testTimelineTiming() {
    this->setTimeline(2, 3, 2, 500);
    this->startExperiment();

    // SPIN_UP: 2 ticks
    this->tick();
    ASSERT_EQ(this->state(), State::EXPERIMENT_DRILLING_SPIN_UP);
    ASSERT_from_PlatformMoveTo_SIZE(0);
    this->tick();
    ASSERT_EQ(this->state(), State::EXPERIMENT_DRILLING_ADVANCE);
    ASSERT_from_PlatformMoveTo_SIZE(1);
    ASSERT_from_PlatformMoveTo(0, 500);
    ASSERT_EVENTS_PhaseAdvance_SIZE(1);
    ASSERT_EVENTS_PhaseAdvance(0, 500, 3);
    ASSERT_from_DrillOFF_SIZE(0);
    // The phase timer restarted on entry
    ASSERT_TLM_PhaseSeconds(this->tlmHistory_PhaseSeconds->size() - 1, 0);

    // ADVANCE: 3 ticks
    this->clearHistory();
    this->tick(2);
    ASSERT_EQ(this->state(), State::EXPERIMENT_DRILLING_ADVANCE);
    ASSERT_EQ(this->hardwareOutputs(), 0U);
    this->tick();
    ASSERT_EQ(this->state(), State::EXPERIMENT_RETRACTING);
    ASSERT_from_DrillOFF_SIZE(1);
    ASSERT_from_PlatformMoveTo_SIZE(1);
    ASSERT_from_PlatformMoveTo(0, 0);
    ASSERT_EVENTS_PhaseRetract_SIZE(1);
    ASSERT_EVENTS_PhaseRetract(0, 2);

    // RETRACTING: 2 ticks
    this->clearHistory();
    this->tick();
    ASSERT_EQ(this->state(), State::EXPERIMENT_RETRACTING);
    this->tick();
    ASSERT_EQ(this->state(), State::EXPERIMENT_DONE);
    ASSERT_EVENTS_PhaseDone_SIZE(1);
    ASSERT_from_PlatformStop_SIZE(0);  // DONE must not halt a retract that is still running
    ASSERT_EQ(this->hardwareOutputs(), 0U);

    // DONE stays put; cameras keep recording
    this->clearHistory();
    this->tick(10);
    ASSERT_EQ(this->state(), State::EXPERIMENT_DONE);
    ASSERT_EQ(this->hardwareOutputs(), 0U);
}

void OrchestratorTester ::testTimelineParamsLoadedOnce() {
    this->setTimeline(2, 3, 2, 500);
    this->startExperiment();

    // Change everything mid-flight
    this->setTimeline(10, 10, 10, 999);

    this->tick(2);
    ASSERT_EQ(this->state(), State::EXPERIMENT_DRILLING_ADVANCE);
    ASSERT_from_PlatformMoveTo(0, 500);
    this->tick(3);
    ASSERT_EQ(this->state(), State::EXPERIMENT_RETRACTING);
}

void OrchestratorTester ::testEodsDuringSpinUpRetracts() {
    this->setTimeline(5, 5, 5, 500);
    this->startExperiment();
    this->tick();

    this->eods();
    ASSERT_EQ(this->state(), State::AFTER_EXPERIMENT);
    ASSERT_from_DrillOFF_SIZE(1);
    ASSERT_from_PlatformMoveTo_SIZE(1);
    ASSERT_from_PlatformMoveTo(0, 0);
    ASSERT_from_OpticalCameraOFF_SIZE(1);
    ASSERT_EVENTS_EnterAfterExperiment_SIZE(1);
}

void OrchestratorTester ::testEodsDuringAdvanceRetracts() {
    this->setTimeline(1, 5, 5, 500);
    this->startExperiment();
    this->tick();
    ASSERT_EQ(this->state(), State::EXPERIMENT_DRILLING_ADVANCE);

    this->clearHistory();
    this->eods();
    ASSERT_EQ(this->state(), State::AFTER_EXPERIMENT);
    ASSERT_from_DrillOFF_SIZE(1);
    ASSERT_from_PlatformMoveTo_SIZE(1);
    ASSERT_from_PlatformMoveTo(0, 0);
    ASSERT_from_OpticalCameraOFF_SIZE(1);
}

void OrchestratorTester ::testEodsAfterDrillingNoSecondRetract() {
    this->setTimeline(1, 1, 5, 500);
    this->startExperiment();
    this->tick(2);
    ASSERT_EQ(this->state(), State::EXPERIMENT_RETRACTING);

    this->clearHistory();
    this->eods();
    ASSERT_EQ(this->state(), State::AFTER_EXPERIMENT);
    ASSERT_from_DrillOFF_SIZE(0);
    ASSERT_from_PlatformMoveTo_SIZE(0);
    ASSERT_from_OpticalCameraOFF_SIZE(1);

    // AFTER_EXPERIMENT ignores further signals
    this->clearHistory();
    this->lo();
    this->soe();
    this->eods();
    this->tick(5);
    ASSERT_EQ(this->state(), State::AFTER_EXPERIMENT);
    ASSERT_EQ(this->hardwareOutputs(), 0U);
}

void OrchestratorTester ::testDrillPositionUnset() {
    this->setTimeline(1, 1, 1, 0);
    this->lo();
    this->soe();
    ASSERT_EVENTS_DrillPositionUnset_SIZE(1);

    this->tick();
    ASSERT_EQ(this->state(), State::EXPERIMENT_DRILLING_ADVANCE);
    ASSERT_from_PlatformMoveTo(0, 0);
    this->tick(2);
    ASSERT_EQ(this->state(), State::EXPERIMENT_DONE);
}

void OrchestratorTester ::testTestMode() {
    ASSERT_EQ(this->command(Command::ENTER_TEST), Fw::CmdResponse::OK);
    ASSERT_EQ(this->state(), State::TEST);
    ASSERT_EVENTS_EnterTest_SIZE(1);
    ASSERT_from_PlatformEnable_SIZE(1);

    this->clearHistory();
    ASSERT_EQ(this->command(Command::DRILL_ON), Fw::CmdResponse::OK);
    ASSERT_from_DrillON_SIZE(1);
    ASSERT_EQ(this->command(Command::DRILL_OFF), Fw::CmdResponse::OK);
    ASSERT_from_DrillOFF_SIZE(1);
    ASSERT_EQ(this->command(Command::PLATFORM_MOVE_TO, 1234), Fw::CmdResponse::OK);
    ASSERT_EQ(this->command(Command::PLATFORM_MOVE_TO, -777), Fw::CmdResponse::OK);
    ASSERT_from_PlatformMoveTo_SIZE(2);
    ASSERT_from_PlatformMoveTo(0, 1234);
    ASSERT_from_PlatformMoveTo(1, -777);
    ASSERT_EQ(this->command(Command::PLATFORM_STOP), Fw::CmdResponse::OK);
    ASSERT_from_PlatformStop_SIZE(1);
    ASSERT_EQ(this->command(Command::CAMERA_ON), Fw::CmdResponse::OK);
    ASSERT_from_OpticalCameraON_SIZE(1);
    ASSERT_EQ(this->command(Command::CAMERA_OFF), Fw::CmdResponse::OK);
    ASSERT_from_OpticalCameraOFF_SIZE(1);
    // No thermal camera component yet: accepted, nothing happens
    ASSERT_EQ(this->command(Command::THERMAL_ON), Fw::CmdResponse::OK);
    ASSERT_EQ(this->command(Command::THERMAL_OFF), Fw::CmdResponse::OK);
    ASSERT_EVENTS_TestCommandIgnored_SIZE(0);

    // LO/SOE/EODS are only logged in TEST (ESA requirement)
    this->clearHistory();
    this->lo();
    this->soe();
    this->eods();
    this->tick(10);
    ASSERT_EQ(this->state(), State::TEST);
    ASSERT_EVENTS_LOTestSignal_SIZE(1);
    ASSERT_EVENTS_SOETestSignal_SIZE(1);
    ASSERT_EVENTS_EODSTestSignal_SIZE(1);
    ASSERT_EVENTS_EnterFlight_SIZE(0);
    ASSERT_EQ(this->hardwareOutputs(), 0U);

    // Leaving TEST puts the hardware in a known state
    this->clearHistory();
    ASSERT_EQ(this->command(Command::EXIT_TEST), Fw::CmdResponse::OK);
    ASSERT_EQ(this->state(), State::IDLE);
    ASSERT_from_DrillOFF_SIZE(1);
    ASSERT_from_OpticalCameraOFF_SIZE(1);
    ASSERT_from_PlatformMoveTo_SIZE(1);
    ASSERT_from_PlatformMoveTo(0, 0);
    ASSERT_from_PlatformStop_SIZE(0);
    ASSERT_EVENTS_ExitTest_SIZE(1);
}

void OrchestratorTester ::testCommandGating() {
    this->setTimeline(5, 5, 5, 500);

    auto checkAllRejected = [this](State expected) {
        for (const Command cmd : TEST_COMMANDS) {
            this->clearHistory();
            ASSERT_EQ(this->command(cmd, 100), Fw::CmdResponse::EXECUTION_ERROR);
            ASSERT_EVENTS_TestCommandIgnored_SIZE(1);
            ASSERT_EQ(this->hardwareOutputs(), 0U);
            ASSERT_EQ(this->state(), expected);
        }
    };

    checkAllRejected(State::IDLE);
    this->lo();
    checkAllRejected(State::FLIGHT);
    this->soe();
    checkAllRejected(State::EXPERIMENT_DRILLING_SPIN_UP);
    this->eods();
    checkAllRejected(State::AFTER_EXPERIMENT);
}

void OrchestratorTester ::testEnterExitGating() {
    ASSERT_EQ(this->command(Command::EXIT_TEST), Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_ExitTestIgnored_SIZE(1);
    ASSERT_EQ(this->state(), State::IDLE);

    ASSERT_EQ(this->command(Command::ENTER_TEST), Fw::CmdResponse::OK);
    this->clearHistory();
    ASSERT_EQ(this->command(Command::ENTER_TEST), Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_EnterTestIgnored_SIZE(1);
    ASSERT_EVENTS_EnterTest_SIZE(0);
    ASSERT_from_PlatformEnable_SIZE(0);  // no second enable
    ASSERT_EQ(this->state(), State::TEST);

    ASSERT_EQ(this->command(Command::EXIT_TEST), Fw::CmdResponse::OK);
    this->lo();
    this->clearHistory();
    ASSERT_EQ(this->command(Command::ENTER_TEST), Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_EnterTestIgnored_SIZE(1);
    ASSERT_EQ(this->command(Command::EXIT_TEST), Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_ExitTestIgnored_SIZE(1);
    ASSERT_EQ(this->state(), State::FLIGHT);
}

void OrchestratorTester ::testTicksOutsideExperiment() {
    this->tick(50);
    ASSERT_EQ(this->state(), State::IDLE);
    ASSERT_EQ(this->hardwareOutputs(), 0U);

    this->command(Command::ENTER_TEST);
    this->clearHistory();
    this->tick(50);
    ASSERT_EQ(this->state(), State::TEST);
    ASSERT_EQ(this->hardwareOutputs(), 0U);

    this->command(Command::EXIT_TEST);
    this->lo();
    this->clearHistory();
    this->tick(50);
    ASSERT_EQ(this->state(), State::FLIGHT);
    ASSERT_EQ(this->hardwareOutputs(), 0U);
}

void OrchestratorTester ::testErrorToSafe() {
    // No transition on error in IDLE or TEST
    this->error();
    ASSERT_EQ(this->state(), State::IDLE);
    this->command(Command::ENTER_TEST);
    this->error();
    ASSERT_EQ(this->state(), State::TEST);
    this->command(Command::EXIT_TEST);

    this->lo();
    this->error();
    ASSERT_EQ(this->state(), State::SAFE);
    ASSERT_EVENTS_EnterSafe_SIZE(1);

    // SAFE ignores everything
    this->clearHistory();
    this->lo();
    this->soe();
    this->eods();
    this->tick(5);
    this->error();
    ASSERT_EQ(this->state(), State::SAFE);
    ASSERT_EQ(this->hardwareOutputs(), 0U);
}

void OrchestratorTester ::testErrorWhileDrillingRetracts() {
    this->setTimeline(1, 5, 5, 500);
    this->startExperiment();
    this->tick();
    ASSERT_EQ(this->state(), State::EXPERIMENT_DRILLING_ADVANCE);

    this->clearHistory();
    this->error();
    ASSERT_EQ(this->state(), State::SAFE);
    ASSERT_from_DrillOFF_SIZE(1);
    ASSERT_from_PlatformMoveTo_SIZE(1);
    ASSERT_from_PlatformMoveTo(0, 0);
}

void OrchestratorTester ::testQueueOverflowDrops() {
    OrchestratorComponentBase& base = this->component;
    const FwSizeType depth = TEST_INSTANCE_QUEUE_DEPTH;  // local copy: gtest binds by reference

    // More commands than the queue holds, without dispatching: must not assert
    const FwSizeType sent = depth + 5;
    for (FwSizeType i = 0; i < sent; i++) {
        this->sendCmd_testPlatformStop(0, static_cast<U32>(i));
    }
    ASSERT_EQ(base.m_queue.getMessagesAvailable(), depth);
    ASSERT_EQ(base.getNumMsgsDropped(), sent - depth);

    // The queued ones are answered; the dropped ones never are
    this->dispatchAll();
    ASSERT_CMD_RESPONSE_SIZE(depth);

    // Ports drop too
    for (FwSizeType i = 0; i < sent; i++) {
        this->invoke_to_schedIn(0, 0);
    }
    ASSERT_EQ(base.getNumMsgsDropped(), 2 * (sent - depth));
    this->dispatchAll();
    ASSERT_EQ(this->state(), State::IDLE);
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void OrchestratorTester ::dispatchAll() {
    OrchestratorComponentBase& base = this->component;
    while (base.m_queue.getMessagesAvailable() > 0) {
        this->component.doDispatch();
    }
}

Fw::CmdResponse OrchestratorTester ::command(Command cmd, I32 position) {
    const FwSizeType before = this->cmdResponseHistory->size();
    const U32 seq = m_cmdSeq++;
    switch (cmd) {
        case Command::ENTER_TEST:
            this->sendCmd_enterTestMode(0, seq);
            break;
        case Command::EXIT_TEST:
            this->sendCmd_exitTestMode(0, seq);
            break;
        case Command::DRILL_ON:
            this->sendCmd_testDrillON(0, seq);
            break;
        case Command::DRILL_OFF:
            this->sendCmd_testDrillOFF(0, seq);
            break;
        case Command::PLATFORM_MOVE_TO:
            this->sendCmd_testPlatformMoveTo(0, seq, position);
            break;
        case Command::PLATFORM_STOP:
            this->sendCmd_testPlatformStop(0, seq);
            break;
        case Command::CAMERA_ON:
            this->sendCmd_testOpticalCameraON(0, seq);
            break;
        case Command::CAMERA_OFF:
            this->sendCmd_testOpticalCameraOFF(0, seq);
            break;
        case Command::THERMAL_ON:
            this->sendCmd_testThermalCameraON(0, seq);
            break;
        case Command::THERMAL_OFF:
            this->sendCmd_testThermalCameraOFF(0, seq);
            break;
    }
    this->dispatchAll();

    // Exactly one response per command
    EXPECT_EQ(this->cmdResponseHistory->size(), before + 1);
    if (this->cmdResponseHistory->size() != before + 1) {
        return Fw::CmdResponse::EXECUTION_ERROR;
    }
    const CmdResponse& response = this->cmdResponseHistory->at(before);
    EXPECT_EQ(response.cmdSeq, seq);
    return response.response;
}

void OrchestratorTester ::lo() {
    this->invoke_to_LOHigh(0);
    this->dispatchAll();
}

void OrchestratorTester ::soe() {
    this->invoke_to_SOEHigh(0);
    this->dispatchAll();
}

void OrchestratorTester ::eods() {
    this->invoke_to_EODSHigh(0);
    this->dispatchAll();
}

void OrchestratorTester ::tick(U32 count) {
    for (U32 i = 0; i < count; i++) {
        this->invoke_to_schedIn(0, 0);
        this->dispatchAll();
    }
}

void OrchestratorTester ::error() {
    OrchestratorComponentBase& base = this->component;
    base.OrchestratorStateMachine_sendSignal_error();
    this->dispatchAll();
}

void OrchestratorTester ::setTimeline(U32 spinUp, U32 advance, U32 retract, I32 drillPosition) {
    this->paramSet_SPIN_UP_SECONDS(spinUp, Fw::ParamValid::VALID);
    this->paramSet_ADVANCE_SECONDS(advance, Fw::ParamValid::VALID);
    this->paramSet_RETRACT_SECONDS(retract, Fw::ParamValid::VALID);
    this->paramSet_DRILL_POSITION(drillPosition, Fw::ParamValid::VALID);
    this->component.loadParameters();
}

OrchestratorTester::State OrchestratorTester ::state() const {
    const OrchestratorComponentBase& base = this->component;
    return base.OrchestratorStateMachine_getState();
}

bool OrchestratorTester ::isDrilling(State s) {
    return s == State::EXPERIMENT_DRILLING_SPIN_UP || s == State::EXPERIMENT_DRILLING_ADVANCE;
}

bool OrchestratorTester ::isTestCommand(Command cmd) {
    return cmd != Command::ENTER_TEST && cmd != Command::EXIT_TEST;
}

FwSizeType OrchestratorTester ::hardwareOutputs() const {
    // Signal ports only keep a count; PlatformMoveTo keeps a history of positions
    return this->fromPortHistorySize_DrillON + this->fromPortHistorySize_DrillOFF +
           this->fromPortHistorySize_OpticalCameraON + this->fromPortHistorySize_OpticalCameraOFF +
           this->fromPortHistorySize_PlatformEnable + this->fromPortHistorySize_PlatformStop +
           this->fromPortHistory_PlatformMoveTo->size();
}

void OrchestratorTester ::checkStep(State before, Step step, Fw::CmdResponse response) {
    const State after = this->state();
    const FwSizeType outputs = this->hardwareOutputs();

    // 1. Hardware only moves on a transition, or for an accepted test command in TEST
    const bool acceptedTestCommand = step == Step::TEST_COMMAND && before == State::TEST;
    if (before == after && !acceptedTestCommand) {
        ASSERT_EQ(outputs, 0U) << "hardware output without a state change";
    }

    // 2. Leaving DRILLING always stops the drill and retracts the platform
    if (isDrilling(before) && !isDrilling(after)) {
        ASSERT_GE(this->fromPortHistorySize_DrillOFF, 1U) << "left DRILLING without DrillOFF";
        ASSERT_GE(this->fromPortHistory_PlatformMoveTo->size(), 1U) << "left DRILLING without a retract";
        ASSERT_EQ(this->fromPortHistory_PlatformMoveTo->at(this->fromPortHistory_PlatformMoveTo->size() - 1).position, 0)
            << "last platform target after DRILLING is not 0";
    }

    // 3. Test commands outside TEST are rejected and do nothing
    if (step == Step::TEST_COMMAND && before != State::TEST) {
        ASSERT_EQ(response, Fw::CmdResponse::EXECUTION_ERROR);
        ASSERT_EQ(outputs, 0U);
        ASSERT_EQ(after, before);
    }

    // 4. In TEST, LO/SOE/EODS and ticks never change the state (ESA requirement)
    if (before == State::TEST && (step == Step::LO || step == Step::SOE || step == Step::EODS || step == Step::TICK)) {
        ASSERT_EQ(after, State::TEST);
    }

    // 5. Outside TEST the flight states only move forward
    if (before != State::TEST && after != State::TEST) {
        ASSERT_GE(flightRank(after), flightRank(before)) << "flight state went backwards";
    }

    // 6. TEST is only entered from IDLE and only left to IDLE
    if (after == State::TEST && before != State::TEST) {
        ASSERT_EQ(before, State::IDLE);
    }
    if (before == State::TEST && after != State::TEST) {
        ASSERT_EQ(after, State::IDLE);
        ASSERT_EQ(step, Step::EXIT_TEST);
    }

    // 7. Reaching AFTER_EXPERIMENT from the experiment closes the recordings
    if (after == State::AFTER_EXPERIMENT && before != State::AFTER_EXPERIMENT) {
        ASSERT_EQ(this->fromPortHistorySize_OpticalCameraOFF, 1U);
    }
}

int OrchestratorTester ::flightRank(State s) {
    switch (s) {
        case State::IDLE:
            return 0;
        case State::FLIGHT:
            return 1;
        case State::EXPERIMENT_DRILLING_SPIN_UP:
            return 2;
        case State::EXPERIMENT_DRILLING_ADVANCE:
            return 3;
        case State::EXPERIMENT_RETRACTING:
            return 4;
        case State::EXPERIMENT_DONE:
            return 5;
        case State::AFTER_EXPERIMENT:
            return 6;
        default:
            return -1;  // TEST, SAFE: not part of the flight order
    }
}

void OrchestratorTester ::startExperiment() {
    this->lo();
    this->soe();
    ASSERT_EQ(this->state(), State::EXPERIMENT_DRILLING_SPIN_UP);
    this->clearHistory();
}

}  // namespace Mara
