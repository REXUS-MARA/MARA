// ======================================================================
// \title  thermalCamera.hpp
// \author aurahs
// \brief  hpp file for thermalCamera component implementation class
// ======================================================================

#ifndef Mara_thermalCamera_HPP
#define Mara_thermalCamera_HPP

#include <sys/types.h>
#include "Mara/Components/thermalCamera/thermalCameraComponentAc.hpp"

namespace Mara {

class thermalCamera final : public thermalCameraComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct thermalCamera object
    thermalCamera(const char* const compName  //!< The component name
    );

    //! Destroy thermalCamera object
    ~thermalCamera();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command start
    //!
    //! command to start camera
    void start_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                          U32 cmdSeq            //!< The command sequence number
                          ) override;

    //! Handler implementation for command stop
    //!
    //! command to start camera
    void stop_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                         U32 cmdSeq            //!< The command sequence number
                         ) override;

    //! Handler implementation for command check
    //!
    //! command to check filesize
    void check_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                          U32 cmdSeq            //!< The command sequence number
                          ) override;

    bool isRecording() const { return m_pid > 0; }
    pid_t m_pid = -1;

};

}  // namespace Mara

#endif
