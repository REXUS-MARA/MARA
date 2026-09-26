// ======================================================================
// \title  OpticalCamera.hpp
// \author claraoberg
// \brief  hpp file for OpticalCamera component implementation class
// ======================================================================

#ifndef Mara_OpticalCamera_HPP
#define Mara_OpticalCamera_HPP

#include "Mara/Components/OpticalCamera/OpticalCameraComponentAc.hpp"
#include "Fw/Types/String.hpp"

namespace Mara {

class OpticalCamera final : public OpticalCameraComponentBase {
   public:
    explicit OpticalCamera(const char* compName);
    ~OpticalCamera() override;

  private:
    void Camera_ON_handler(FwIndexType portNum) override;
    void Camera_OFF_handler(FwIndexType portNum) override;
    void pingIn_handler(FwIndexType portNum, U32 key) override;

    void checkRecorder();
    void stopRecorder();
    bool isRecording() const { return m_pid > 0; }

    pid_t m_pid = -1;
    U32 m_segment = 0;
    Fw::String m_primaryPath;  // path the running ffmpeg was started with
    FwSizeType m_lastSize = 0;
    U32 m_stalls = 0;
    static const U32 STALL_PINGS = 5;
};

}  // namespace Mara

#endif
