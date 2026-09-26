// ======================================================================
// \title  OpticalCamera.cpp
// \author claraoberg
// \brief  cpp file for OpticalCamera component implementation class
// ======================================================================

#include "Mara/Components/OpticalCamera/OpticalCamera.hpp"
#include "Os/FileSystem.hpp"

#include <array>
#include <cinttypes>
#include <csignal>
#include <spawn.h>
#include <sys/wait.h>

extern char** environ;

namespace Mara {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

OpticalCamera ::OpticalCamera(const char* const compName) : OpticalCameraComponentBase(compName) {}

OpticalCamera ::~OpticalCamera() {
    stopRecorder();
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void OpticalCamera ::Camera_ON_handler(FwIndexType portNum) {
    
    checkRecorder();
    if (isRecording()) {
        return;
    }

    Fw::ParamValid valid;
    const auto device = this->paramGet_DEVICE(valid);
    const auto videoSize = this->paramGet_VIDEO_SIZE(valid);
    const auto primaryDir = this->paramGet_PRIMARY_DIR(valid);
    const auto backupDir = this->paramGet_BACKUP_DIR(valid);

    Fw::String framerate;
    framerate.format("%" PRIu32, this->paramGet_FRAMERATE(valid));
    Fw::String maxSeconds;
    maxSeconds.format("%" PRIu32, this->paramGet_MAX_SECONDS(valid));

    // New segment per ON, so a second ON/OFF cycle never overwrites the first.
    m_segment++;
    m_primaryPath.format("%s/cam_%03" PRIu32 ".mkv", primaryDir.toChar(), m_segment);
    Fw::String teeSpec;
    teeSpec.format("[f=matroska]%s|[f=matroska]%s/cam_%03" PRIu32 ".mkv", m_primaryPath.toChar(),
                   backupDir.toChar(), m_segment);

    const std::array<const char*, 24> argv{
        "ffmpeg", "-nostdin", "-loglevel", "error",
        "-f", "v4l2", "-input_format", "mjpeg",
        "-video_size", videoSize.toChar(), "-framerate", framerate.toChar(),
        "-i", device.toChar(),
        "-map", "0:v", "-c", "copy",   // no re-encoding
        "-t", maxSeconds.toChar(),     // stops by itself after MAX_SECONDS
        "-f", "tee", teeSpec.toChar(), // writes to both cards
        nullptr};

    const int err = posix_spawnp(&m_pid, "ffmpeg", nullptr, nullptr, const_cast<char* const*>(argv.data()), environ);
    if (err != 0) {
        m_pid = -1;
        this->log_WARNING_HI_SpawnFailed(err);
        return;
    }

    m_lastSize = 0;
    m_stalls = 0;
    this->log_ACTIVITY_HI_RecordingStarted(m_segment);
}



void OpticalCamera::Camera_OFF_handler(FwIndexType portNum) {
    (void)portNum;

    checkRecorder();
    if (isRecording()) {
        stopRecorder();
        this->log_ACTIVITY_HI_RecordingStopped(m_segment);
    }
}

void OpticalCamera::pingIn_handler(FwIndexType portNum, U32 key) {
    (void)portNum;
    checkRecorder();
    this->pingOut_out(0, key);  // always answer, or Health goes FATAL
}

void OpticalCamera::checkRecorder() {
    if (!isRecording()) {
        return;
    }

    int status = 0;
    if (waitpid(m_pid, &status, WNOHANG) == m_pid) {
        m_pid = -1;
        // Exit code 0 is normal: MAX_SECONDS was reached.
        if (!(WIFEXITED(status) && WEXITSTATUS(status) == 0)) {
            this->log_WARNING_HI_RecorderExitedEarly(m_segment, status);
        }
        return;
    }

    FwSizeType size = 0;
    (void)Os::FileSystem::getFileSize(m_primaryPath.toChar(), size);
    if (size > m_lastSize) {
        m_lastSize = size;
        m_stalls = 0;
    } else if (++m_stalls == STALL_PINGS) {
        this->log_WARNING_HI_RecorderStalled(m_segment, size);
    }
}

void OpticalCamera::stopRecorder() {
    if (!isRecording()) {
        return;
    }
    (void)kill(m_pid, SIGINT);  // ffmpeg finalises the files and exits
    (void)waitpid(m_pid, nullptr, 0);
    m_pid = -1;
}

}  // namespace Mara
