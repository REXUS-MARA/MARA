// ======================================================================
// \title  OpticalCamera.cpp
// \author claraoberg
// \brief  cpp file for OpticalCamera component implementation class
// ======================================================================

#include "Mara/Components/OpticalCamera/OpticalCamera.hpp"

namespace Mara {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

OpticalCamera ::OpticalCamera(const char* const compName) : OpticalCameraComponentBase(compName) {}

OpticalCamera ::~OpticalCamera() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void OpticalCamera ::TODO_handler(FwIndexType portNum, U32 context) {
    // TODO
}

}  // namespace Mara
