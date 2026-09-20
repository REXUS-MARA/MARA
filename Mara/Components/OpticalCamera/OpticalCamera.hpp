// ======================================================================
// \title  OpticalCamera.hpp
// \author claraoberg
// \brief  hpp file for OpticalCamera component implementation class
// ======================================================================

#ifndef Mara_OpticalCamera_HPP
#define Mara_OpticalCamera_HPP

#include "Mara/Components/OpticalCamera/OpticalCameraComponentAc.hpp"

namespace Mara {

class OpticalCamera final : public OpticalCameraComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct OpticalCamera object
    OpticalCamera(const char* const compName  //!< The component name
    );

    //! Destroy OpticalCamera object
    ~OpticalCamera();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for Camera_ON
    //!
    //! Turns on the camera recording
    void Camera_ON_handler(FwIndexType portNum,  //!< The port number
                      U32 context           //!< The call order
                      ) override;

    //! Handler implementation for Camera_OFF
    //!
    //! Turns off the camera recording
    void Camera_OFF_handler(FwIndexType portNum,  //!< The port number
                       U32 context            //!< The call order
                       ) override;
};

}  // namespace Mara

#endif
