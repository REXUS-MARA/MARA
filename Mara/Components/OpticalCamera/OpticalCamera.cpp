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

void OpticalCamera ::Camera_ON_handler(FwIndexType portNum, U32 context) {
    // TODO
}

void OpticalCamera ::Camera_OFF_handler(FwIndexType portNum, U32 context) {
    // TODO
}

}  // namespace Mara
