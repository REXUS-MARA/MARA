// ======================================================================
// \title  FatalHandler.cpp
// \author fsowa
// \brief  cpp file for FatalHandler component implementation class
// ======================================================================

#include "Mara/Components/FatalHandler/FatalHandler.hpp"

namespace Mara {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

FatalHandler ::FatalHandler(const char* const compName) : FatalHandlerComponentBase(compName) {}

FatalHandler ::~FatalHandler() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void FatalHandler ::FatalReceive_handler(FwIndexType portNum, FwEventIdType Id) {
    (void)portNum;
    // Called on whichever thread logged the FATAL, so the count is atomic.
    // Deliberately no abort and no exit: the software carries on.
    const U32 count = ++m_fatalCount;
    this->log_WARNING_HI_FatalSurvived(Id, count);
    this->tlmWrite_FatalCount(count);
}

}  // namespace Mara
