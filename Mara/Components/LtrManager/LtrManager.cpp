// ======================================================================
// \title  LtrManager.cpp
// \author fsowa
// \brief  cpp file for LtrManager component implementation class
// ======================================================================

#include "Mara/Components/LtrManager/LtrManager.hpp"

namespace Mara {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

LtrManager ::LtrManager(const char* const compName) : LtrManagerComponentBase(compName) {}

LtrManager ::~LtrManager() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void LtrManager ::run_handler(FwIndexType portNum, U32 context) {
    // TODO
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void LtrManager ::RESET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void LtrManager ::Mara_I2CSensorStateMachine_action_doReset(SmId smId, Mara_I2CSensorStateMachine::Signal signal) {
    // TODO
}

void LtrManager ::Mara_I2CSensorStateMachine_action_checkReset(SmId smId, Mara_I2CSensorStateMachine::Signal signal) {
    // TODO
}

void LtrManager ::Mara_I2CSensorStateMachine_action_doEnable(SmId smId, Mara_I2CSensorStateMachine::Signal signal) {
    // TODO
}

void LtrManager ::Mara_I2CSensorStateMachine_action_doConfigure(SmId smId, Mara_I2CSensorStateMachine::Signal signal) {
    // TODO
}

void LtrManager ::Mara_I2CSensorStateMachine_action_doRead(SmId smId, Mara_I2CSensorStateMachine::Signal signal) {
    // TODO
}

}  // namespace Mara
