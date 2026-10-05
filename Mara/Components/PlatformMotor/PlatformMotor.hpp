// ======================================================================
// \title  PlatformMotor.hpp
// \author fsowa
// \brief  hpp file for PlatformMotor component implementation class
// ======================================================================

#ifndef Mara_PlatformMotor_HPP
#define Mara_PlatformMotor_HPP

#include "Mara/Components/PlatformMotor/PlatformMotorComponentAc.hpp"

namespace Mara {

class PlatformMotor final : public PlatformMotorComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct PlatformMotor object
    PlatformMotor(const char* const compName  //!< The component name
    );

    //! Destroy PlatformMotor object
    ~PlatformMotor();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for byteStreamDriverReady
    //!
    //! Port for receiving ready signals from the driver
    //! Sample connection: byteStreamDriver.ready -> byteStreamDriverClient.byteStreamDriverReady
    void byteStreamDriverReady_handler(FwIndexType portNum  //!< The port number
                                       ) override;

    //! Handler implementation for enable
    //!
    //! Enable the drive. Also clears a drive fault.
    void enable_handler(FwIndexType portNum  //!< The port number
                        ) override;

    //! Handler implementation for fromByteStreamDriver
    //!
    //! Port for receiving data from the driver
    //! Sample connection: byteStreamDriver.$recv -> byteStreamDriverClient.fromDriver
    void fromByteStreamDriver_handler(FwIndexType portNum,  //!< The port number
                                      Fw::Buffer& buffer,
                                      const Drv::ByteStreamStatus& status) override;

    //! Handler implementation for moveTo
    //!
    //! Move to an absolute position in encoder counts, clamped to [MIN_POSITION, MAX_POSITION].
    //! 0 is the position at drive power-up.
    void moveTo_handler(FwIndexType portNum,  //!< The port number
                        I32 position) override;

    //! Handler implementation for pingIn
    //!
    //! Health ping
    void pingIn_handler(FwIndexType portNum,  //!< The port number
                        U32 key               //!< Value to return to pinger
                        ) override;

    //! Handler implementation for stop
    //!
    //! Stop and hold position
    void stop_handler(FwIndexType portNum  //!< The port number
                      ) override;

  private:
    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Write one value into the drive's object dictionary (CANopen SDO download).
    //! \param size value size in bytes: 1, 2 or 4
    void sdoWrite(U16 index, U32 value, U8 size);

    I32 m_target = 0;  //!< Last commanded target; 0 is the power-up position
};

}  // namespace Mara

#endif
