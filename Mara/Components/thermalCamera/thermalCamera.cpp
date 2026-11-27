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
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void thermalCamera ::cameraOff_handler(FwIndexType portNum) {
    this->startThatShit();
}

void thermalCamera ::cameraOn_handler(FwIndexType portNum) {
    this->stop();
}

void thermalCamera ::checkCamera_handler(FwIndexType portNum) {
    this->check();
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void thermalCamera ::start_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->startThatShit();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void thermalCamera ::stop_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->stop();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void thermalCamera ::check_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->check();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void thermalCamera ::startThatShit() { // i will actually kill the fpp compiler. what is wrong with "start()"
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

void thermalCamera ::stop() {
    if(true == this->isRecording()) {
        (void)kill(m_pid, SIGKILL);
        this->log_ACTIVITY_HI_RecordingStopped();
    }
}

void thermalCamera ::check() {
    struct stat st;
    stat(file1, &st);
    this->log_ACTIVITY_HI_ReportFileSize(st.st_size/1000);
    stat(file2, &st);
    this->log_ACTIVITY_HI_ReportFileSize(st.st_size/1000);
}

}  // namespace Mara
