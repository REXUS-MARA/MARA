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
    char *argV[] = {(const char*)"/bin/python3", (const char*)"/home/mara/camera.py", 0};
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
    stat("/home/mara/thermaldata.dat", &st);
    this->log_ACTIVITY_HI_ReportFileSize(st.st_size/1000);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace Mara
