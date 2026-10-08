// ======================================================================
// \title  Orchestrator.cpp
// \author fsowa
// \brief  cpp file for Orchestrator component implementation class
// ======================================================================

#include "Mara/Components/Orchestrator/Orchestrator.hpp"

namespace Mara {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

Orchestrator ::Orchestrator(const char* const compName) : OrchestratorComponentBase(compName) {}

Orchestrator ::~Orchestrator() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void Orchestrator ::EODSHigh_handler(FwIndexType portNum) {
    this->OrchestratorStateMachine_sendSignal_EODS();
}

void Orchestrator ::LOHigh_handler(FwIndexType portNum) {
    this->OrchestratorStateMachine_sendSignal_LO();
}

void Orchestrator ::SOEHigh_handler(FwIndexType portNum) {
    this->OrchestratorStateMachine_sendSignal_SOE();
}

void Orchestrator ::schedIn_handler(FwIndexType portNum, U32 context) {
    m_phaseSeconds++;
    this->tlmWrite_PhaseSeconds(m_phaseSeconds);
    // States that don't handle tick ignore it
    this->OrchestratorStateMachine_sendSignal_tick();
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void Orchestrator ::enterTestMode_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (this->OrchestratorStateMachine_getState() != Mara_OrchestratorStateMachine::State::IDLE) {
        this->log_WARNING_LO_EnterTestIgnored();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    this->OrchestratorStateMachine_sendSignal_EnterTest();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void Orchestrator ::exitTestMode_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (this->OrchestratorStateMachine_getState() != Mara_OrchestratorStateMachine::State::TEST) {
        this->log_WARNING_LO_ExitTestIgnored();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    this->OrchestratorStateMachine_sendSignal_ExitTest();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void Orchestrator ::testDrillON_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->acceptTestCommand(opCode, cmdSeq)) {
        return;
    }
    this->OrchestratorStateMachine_sendSignal_testDrillON();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void Orchestrator ::testDrillOFF_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->acceptTestCommand(opCode, cmdSeq)) {
        return;
    }
    this->OrchestratorStateMachine_sendSignal_testDrillOFF();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void Orchestrator ::testPlatformMoveTo_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, I32 position) {
    if (!this->acceptTestCommand(opCode, cmdSeq)) {
        return;
    }
    this->OrchestratorStateMachine_sendSignal_testPlatformMoveTo(position);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void Orchestrator ::testPlatformStop_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->acceptTestCommand(opCode, cmdSeq)) {
        return;
    }
    this->OrchestratorStateMachine_sendSignal_testPlatformStop();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void Orchestrator ::testOpticalCameraON_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->acceptTestCommand(opCode, cmdSeq)) {
        return;
    }
    this->OrchestratorStateMachine_sendSignal_testOpticalCameraON();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void Orchestrator ::testOpticalCameraOFF_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->acceptTestCommand(opCode, cmdSeq)) {
        return;
    }
    this->OrchestratorStateMachine_sendSignal_testOpticalCameraOFF();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void Orchestrator ::testThermalCameraON_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->acceptTestCommand(opCode, cmdSeq)) {
        return;
    }
    this->OrchestratorStateMachine_sendSignal_testThermalCameraON();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void Orchestrator ::testThermalCameraOFF_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->acceptTestCommand(opCode, cmdSeq)) {
        return;
    }
    this->OrchestratorStateMachine_sendSignal_testThermalCameraOFF();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void Orchestrator ::Mara_OrchestratorStateMachine_action_drillOn(SmId smId,
                                                                 Mara_OrchestratorStateMachine::Signal signal) {
    if (this->isConnected_DrillON_OutputPort(0)) {
        this->DrillON_out(0);
    }
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_drillOff(SmId smId,
                                                                  Mara_OrchestratorStateMachine::Signal signal) {
    if (this->isConnected_DrillOFF_OutputPort(0)) {
        this->DrillOFF_out(0);
    }
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_cameraOn(SmId smId,
                                                                  Mara_OrchestratorStateMachine::Signal signal) {
    if (this->isConnected_OpticalCameraON_OutputPort(0)) {
        this->OpticalCameraON_out(0);
    }
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_cameraOff(SmId smId,
                                                                   Mara_OrchestratorStateMachine::Signal signal) {
    if (this->isConnected_OpticalCameraOFF_OutputPort(0)) {
        this->OpticalCameraOFF_out(0);
    }
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_platformEnable(SmId smId,
                                                                        Mara_OrchestratorStateMachine::Signal signal) {
    if (this->isConnected_PlatformEnable_OutputPort(0)) {
        this->PlatformEnable_out(0);
    }
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_platformAdvance(SmId smId,
                                                                         Mara_OrchestratorStateMachine::Signal signal) {
    if (this->isConnected_PlatformMoveTo_OutputPort(0)) {
        this->PlatformMoveTo_out(0, m_drillPosition);
    }
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_platformRetract(SmId smId,
                                                                         Mara_OrchestratorStateMachine::Signal signal) {
    if (this->isConnected_PlatformMoveTo_OutputPort(0)) {
        this->PlatformMoveTo_out(0, 0);
    }
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_platformStop(SmId smId,
                                                                      Mara_OrchestratorStateMachine::Signal signal) {
    if (this->isConnected_PlatformStop_OutputPort(0)) {
        this->PlatformStop_out(0);
    }
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_doTestPlatformMoveTo(
    SmId smId,
    Mara_OrchestratorStateMachine::Signal signal,
    I32 value) {
    if (this->isConnected_PlatformMoveTo_OutputPort(0)) {
        this->PlatformMoveTo_out(0, value);
    }
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_doTestThermalCameraON(
    SmId smId,
    Mara_OrchestratorStateMachine::Signal signal) {
    // TODO: no thermal camera component yet
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_doTestThermalCameraOFF(
    SmId smId,
    Mara_OrchestratorStateMachine::Signal signal) {
    // TODO: no thermal camera component yet
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_doTestLO(SmId smId,
                                                                  Mara_OrchestratorStateMachine::Signal signal) {
    this->log_ACTIVITY_HI_LOTestSignal();
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_doTestSOE(SmId smId,
                                                                   Mara_OrchestratorStateMachine::Signal signal) {
    this->log_ACTIVITY_HI_SOETestSignal();
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_doTestEODS(SmId smId,
                                                                    Mara_OrchestratorStateMachine::Signal signal) {
    this->log_ACTIVITY_HI_EODSTestSignal();
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_loadTimeline(SmId smId,
                                                                      Mara_OrchestratorStateMachine::Signal signal) {
    Fw::ParamValid valid;
    m_spinUpSeconds = this->paramGet_SPIN_UP_SECONDS(valid);
    m_advanceSeconds = this->paramGet_ADVANCE_SECONDS(valid);
    m_retractSeconds = this->paramGet_RETRACT_SECONDS(valid);
    m_drillPosition = this->paramGet_DRILL_POSITION(valid);
    if (m_drillPosition == 0) {
        this->log_WARNING_HI_DrillPositionUnset();
    }
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_resetPhaseTimer(SmId smId,
                                                                         Mara_OrchestratorStateMachine::Signal signal) {
    m_phaseSeconds = 0;
    this->tlmWrite_PhaseSeconds(m_phaseSeconds);
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_notifyEnterTEST(SmId smId,
                                                                         Mara_OrchestratorStateMachine::Signal signal) {
    this->log_ACTIVITY_HI_EnterTest();
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_notifyExitTEST(SmId smId,
                                                                        Mara_OrchestratorStateMachine::Signal signal) {
    this->log_ACTIVITY_HI_ExitTest();
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_notifyEnterFlight(
    SmId smId,
    Mara_OrchestratorStateMachine::Signal signal) {
    this->log_ACTIVITY_HI_EnterFlight();
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_notifyEnterExperiment(
    SmId smId,
    Mara_OrchestratorStateMachine::Signal signal) {
    this->log_ACTIVITY_HI_EnterExperiment();
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_notifySpinUp(SmId smId,
                                                                      Mara_OrchestratorStateMachine::Signal signal) {
    this->log_ACTIVITY_HI_PhaseSpinUp(m_spinUpSeconds);
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_notifyAdvance(SmId smId,
                                                                       Mara_OrchestratorStateMachine::Signal signal) {
    this->log_ACTIVITY_HI_PhaseAdvance(m_drillPosition, m_advanceSeconds);
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_notifyRetract(SmId smId,
                                                                       Mara_OrchestratorStateMachine::Signal signal) {
    this->log_ACTIVITY_HI_PhaseRetract(m_retractSeconds);
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_notifyDone(SmId smId,
                                                                    Mara_OrchestratorStateMachine::Signal signal) {
    this->log_ACTIVITY_HI_PhaseDone();
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_notifyEnterAfterExperiment(
    SmId smId,
    Mara_OrchestratorStateMachine::Signal signal) {
    this->log_ACTIVITY_HI_EnterAfterExperiment();
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_notifyEnterSafe(SmId smId,
                                                                         Mara_OrchestratorStateMachine::Signal signal) {
    this->log_ACTIVITY_HI_EnterSafe();
}

// ----------------------------------------------------------------------
// Implementations for internal state machine guards
// ----------------------------------------------------------------------

bool Orchestrator ::Mara_OrchestratorStateMachine_guard_spinUpDone(SmId smId,
                                                                   Mara_OrchestratorStateMachine::Signal signal) const {
    return m_phaseSeconds >= m_spinUpSeconds;
}

bool Orchestrator ::Mara_OrchestratorStateMachine_guard_advanceDone(
    SmId smId,
    Mara_OrchestratorStateMachine::Signal signal) const {
    return m_phaseSeconds >= m_advanceSeconds;
}

bool Orchestrator ::Mara_OrchestratorStateMachine_guard_retractDone(
    SmId smId,
    Mara_OrchestratorStateMachine::Signal signal) const {
    return m_phaseSeconds >= m_retractSeconds;
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

bool Orchestrator ::acceptTestCommand(FwOpcodeType opCode, U32 cmdSeq) {
    if (this->OrchestratorStateMachine_getState() == Mara_OrchestratorStateMachine::State::TEST) {
        return true;
    }
    this->log_WARNING_LO_TestCommandIgnored();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    return false;
}

}  // namespace Mara
