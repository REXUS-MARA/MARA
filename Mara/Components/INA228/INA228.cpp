// ======================================================================
// \title  INA228.cpp
// \author sebahs
// \brief  cpp file for INA228 component implementation class
// ======================================================================

#include "Mara/Components/INA228/INA228.hpp"

namespace Mara {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

INA228 ::INA228(const char* const compName) : INA228ComponentBase(compName) {}

INA228 ::~INA228() {}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void INA228 ::TODO_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace Mara
