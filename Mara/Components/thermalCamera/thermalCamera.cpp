// ======================================================================
// \title  thermalCamera.cpp
// \author sebahs
// \brief  cpp file for thermalCamera component implementation class
// ======================================================================

#include "Mara/Components/thermalCamera/thermalCamera.hpp"

#include <spawn.h>
#include <csignal>
#include <sys/stat.h>

extern char** environ;

namespace Mara {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

thermalCamera ::thermalCamera(const char* const compName) : thermalCameraComponentBase(compName) {}

thermalCamera ::~thermalCamera() {}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void thermalCamera ::start_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    // make the cpp compiler shut the hell up
    char python[] = "/bin/python3";
    char camerapath[] = "/home/mara/camera.py";
    // like, what was wring with doing it the C way? smh my head
    char *argV[] = {python, camerapath, file1, file2, 0};
    if(false == this->isRecording()) {
        if(0 != posix_spawn(&m_pid, "/bin/python3", NULL, NULL, argV, environ)) {
            m_pid = -1;
        }
        this->log_ACTIVITY_HI_RecordingStarted();
    }
}

void thermalCamera ::stop_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if(true == this->isRecording()) {
        (void)kill(m_pid, SIGKILL);
        this->log_ACTIVITY_HI_RecordingStopped();
    }
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void thermalCamera ::check_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    struct stat st;
    stat(file1, &st);
    this->log_ACTIVITY_HI_ReportFileSize(st.st_size/1000);
    stat(file2, &st);
    this->log_ACTIVITY_HI_ReportFileSize(st.st_size/1000);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace Mara
