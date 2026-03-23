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
// Handler implementations for commands
// ----------------------------------------------------------------------

void LtrManager ::TODO_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace Mara
