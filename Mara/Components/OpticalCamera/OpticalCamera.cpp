// ======================================================================
// \title  OpticalCamera.cpp
// \author claraoberg
// \brief  cpp file for OpticalCamera component implementation class
// ======================================================================

#include "Mara/Components/OpticalCamera/OpticalCamera.hpp"
#include "Os/FileSystem.hpp"
#include "Os/Task.hpp"

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
    stopRecorder(false);  // no events: at teardown the event components may already be gone
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

    // Start ffmpeg with default SIGINT/SIGTERM handling and nothing blocked, whatever this process
    // inherited: a program started from a background shell has SIGINT ignored, and OFF relies on it.
    posix_spawnattr_t attr;
    (void)posix_spawnattr_init(&attr);
    sigset_t defaults;
    (void)sigemptyset(&defaults);
    (void)sigaddset(&defaults, SIGINT);
    (void)sigaddset(&defaults, SIGTERM);
    (void)posix_spawnattr_setsigdefault(&attr, &defaults);
    sigset_t noneBlocked;
    (void)sigemptyset(&noneBlocked);
    (void)posix_spawnattr_setsigmask(&attr, &noneBlocked);
    (void)posix_spawnattr_setflags(&attr, static_cast<short>(POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK));

    const int err = posix_spawnp(&m_pid, "ffmpeg", nullptr, &attr, const_cast<char* const*>(argv.data()), environ);
    (void)posix_spawnattr_destroy(&attr);
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

void OpticalCamera::stopRecorder(bool report) {
    if (!isRecording()) {
        return;
    }
    Fw::ParamValid valid;
    const U32 timeoutSeconds = this->paramGet_STOP_TIMEOUT_SECONDS(valid);

    (void)kill(m_pid, SIGINT);  // ffmpeg finalises the files and exits
    if (!waitForExit(timeoutSeconds * 1000)) {
        // Not stopping (e.g. the camera wedged): kill it rather than block this component forever
        (void)kill(m_pid, SIGKILL);
        if (report) {
            this->log_WARNING_HI_RecorderKilled(m_segment, timeoutSeconds);
        }
        if (!waitForExit(KILL_TIMEOUT_MS) && report) {
            // Stuck in the kernel (uninterruptible). Stop tracking it; a new ON starts a new ffmpeg.
            this->log_WARNING_HI_RecorderUnresponsive(m_segment);
        }
    }
    m_pid = -1;
}

bool OpticalCamera::waitForExit(U32 timeoutMs) {
    for (U32 waited = 0;; waited += POLL_MS) {
        const pid_t result = waitpid(m_pid, nullptr, WNOHANG);
        if (result == m_pid || result < 0) {
            return true;  // reaped (or not our child any more)
        }
        if (waited >= timeoutMs) {
            return false;
        }
        (void)Os::Task::delay(Fw::TimeInterval(0, POLL_MS * 1000));
    }
}

}  // namespace Mara
