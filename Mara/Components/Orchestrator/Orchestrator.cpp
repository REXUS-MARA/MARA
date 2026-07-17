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
// Handler implementations for commands
// ----------------------------------------------------------------------

void Orchestrator ::enterTestMode_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    OrchestratorStateMachine_sendSignal_EnterTest();
    cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void Orchestrator ::exitTestMode_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    OrchestratorStateMachine_sendSignal_ExitTest();
    cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void Orchestrator ::testDrill_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    OrchestratorStateMachine_sendSignal_testDrill();
    cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void Orchestrator ::testPlatform_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    OrchestratorStateMachine_sendSignal_testPlatform();
    cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void Orchestrator ::Mara_OrchestratorStateMachine_action_doTestDrill(SmId smId,
                                                                     Mara_OrchestratorStateMachine::Signal signal) {
    // TODO
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_doTestPlatform(SmId smId,
                                                                        Mara_OrchestratorStateMachine::Signal signal) {
    // TODO
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_doTestLO(SmId smId,
                                                                  Mara_OrchestratorStateMachine::Signal signal) {
    // TODO
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_doTestSOE(SmId smId,
                                                                   Mara_OrchestratorStateMachine::Signal signal) {
    // TODO
}

void Orchestrator ::Mara_OrchestratorStateMachine_action_doTestEODS(SmId smId,
                                                                    Mara_OrchestratorStateMachine::Signal signal) {
    // TODO
}

}  // namespace Mara
