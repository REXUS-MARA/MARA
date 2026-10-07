// ======================================================================
// \title  OpticalCameraTester.cpp
// \author fsowa
// \brief  cpp file for OpticalCamera component test harness implementation class
// ======================================================================

#include "OpticalCameraTester.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>

namespace Mara {

namespace {
//! Fake ffmpeg. Logs lifecycle to $FAKE_FFMPEG_LOG and the arguments of the latest run to
//! $FAKE_FFMPEG_LOG.args. The primary output path is taken from the tee spec (last argument).
constexpr const char* FAKE_FFMPEG = R"SH(#!/bin/sh
log="$FAKE_FFMPEG_LOG"
: > "$log.args"
for a in "$@"; do printf '%s\n' "$a" >> "$log.args"; done
echo start >> "$log"
spec=""
for a in "$@"; do spec="$a"; done
primary="${spec#\[f=matroska\]}"
primary="${primary%%|*}"
trap 'echo sigint >> "$log"; exit 0' INT
case "$FAKE_FFMPEG_MODE" in
  exit0) echo exit0 >> "$log"; exit 0 ;;
  exit1) echo exit1 >> "$log"; exit 1 ;;
  stall) echo frame >> "$primary"; while true; do sleep 0.05; done ;;
  *) while true; do echo frame >> "$primary"; sleep 0.05; done ;;
esac
)SH";

std::string makeTempDir() {
    std::string pattern = "/tmp/mara_camera_ut_XXXXXX";
    std::vector<char> buffer(pattern.begin(), pattern.end());
    buffer.push_back('\0');
    const char* dir = ::mkdtemp(buffer.data());
    return (dir != nullptr) ? std::string(dir) : std::string();
}

bool contains(const std::vector<std::string>& lines, const std::string& value) {
    for (const std::string& line : lines) {
        if (line == value) {
            return true;
        }
    }
    return false;
}

//! The value following a flag in an argument list, or "" if absent
std::string argAfter(const std::vector<std::string>& args, const std::string& flag) {
    for (size_t i = 0; i + 1 < args.size(); i++) {
        if (args[i] == flag) {
            return args[i + 1];
        }
    }
    return "";
}
}  // namespace

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

OpticalCameraTester ::OpticalCameraTester()
    : OpticalCameraGTestBase("OpticalCameraTester", OpticalCameraTester::MAX_HISTORY_SIZE), component("OpticalCamera") {
    this->initComponents();
    this->connectPorts();

    m_dir = makeTempDir();
    EXPECT_FALSE(m_dir.empty());
    m_primary = m_dir + "/sd1";
    m_backup = m_dir + "/sd2";
    EXPECT_EQ(::mkdir(m_primary.c_str(), 0755), 0);
    EXPECT_EQ(::mkdir(m_backup.c_str(), 0755), 0);
    const std::string binDir = m_dir + "/bin";
    EXPECT_EQ(::mkdir(binDir.c_str(), 0755), 0);

    // Install the fake first on PATH; keep the system dirs for sh and sleep
    const std::string fake = binDir + "/ffmpeg";
    {
        std::ofstream script(fake);
        script << FAKE_FFMPEG;
    }
    EXPECT_EQ(::chmod(fake.c_str(), 0755), 0);
    const char* path = ::getenv("PATH");
    m_savedPath = (path != nullptr) ? path : "";
    EXPECT_EQ(::setenv("PATH", (binDir + ":/usr/bin:/bin").c_str(), 1), 0);
    EXPECT_EQ(::setenv("FAKE_FFMPEG_LOG", this->logPath().c_str(), 1), 0);
    this->setMode("record");

    this->paramSet_DEVICE(Fw::ParamString("/dev/video-test"), Fw::ParamValid::VALID);
    this->paramSet_VIDEO_SIZE(Fw::ParamString("640x480"), Fw::ParamValid::VALID);
    this->paramSet_FRAMERATE(15, Fw::ParamValid::VALID);
    this->paramSet_PRIMARY_DIR(Fw::ParamString(m_primary.c_str()), Fw::ParamValid::VALID);
    this->paramSet_BACKUP_DIR(Fw::ParamString(m_backup.c_str()), Fw::ParamValid::VALID);
    this->paramSet_MAX_SECONDS(42, Fw::ParamValid::VALID);
    this->component.loadParameters();
}

OpticalCameraTester ::~OpticalCameraTester() {
    (void)::setenv("PATH", m_savedPath.c_str(), 1);
    // The temp dir is left for inspection; the component's destructor stops any recorder.
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void OpticalCameraTester ::testOnOffArguments() {
    this->startRecording();
    ASSERT_EVENTS_RecordingStarted_SIZE(1);
    ASSERT_EVENTS_RecordingStarted(0, 1);

    const std::vector<std::string> args = this->lastArgs();
    ASSERT_EQ(argAfter(args, "-f"), "v4l2");
    ASSERT_EQ(argAfter(args, "-input_format"), "mjpeg");
    ASSERT_EQ(argAfter(args, "-video_size"), "640x480");
    ASSERT_EQ(argAfter(args, "-framerate"), "15");
    ASSERT_EQ(argAfter(args, "-i"), "/dev/video-test");
    ASSERT_EQ(argAfter(args, "-c"), "copy") << "must not re-encode";
    ASSERT_EQ(argAfter(args, "-t"), "42");
    ASSERT_TRUE(contains(args, "-nostdin"));
    const std::string tee = "[f=matroska]" + m_primary + "/cam_001.mkv|[f=matroska]" + m_backup + "/cam_001.mkv";
    ASSERT_EQ(args.back(), tee);
    ASSERT_EQ(args[args.size() - 2], "tee");

    this->off();
    ASSERT_EVENTS_RecordingStopped_SIZE(1);
    ASSERT_EVENTS_RecordingStopped(0, 1);
    // OFF waits for ffmpeg, so the SIGINT has been handled by now
    ASSERT_TRUE(contains(this->logLines(), "sigint"));
}

void OpticalCameraTester ::testOnIdempotentAndSegments() {
    this->startRecording();
    this->on();  // already recording
    ASSERT_EVENTS_RecordingStarted_SIZE(1);

    std::vector<std::string> lines = this->logLines();
    FwSizeType starts = 0;
    for (const std::string& line : lines) {
        starts += (line == "start") ? 1 : 0;
    }
    ASSERT_EQ(starts, 1U) << "a second ON must not start a second ffmpeg";

    this->off();
    this->on();
    ASSERT_EVENTS_RecordingStarted_SIZE(2);
    ASSERT_EVENTS_RecordingStarted(1, 2);
    ASSERT_TRUE(this->waitForFile(m_primary + "/cam_002.mkv", 2000)) << "segment 2 must use a new file";
    ASSERT_TRUE(this->waitForFile(m_primary + "/cam_001.mkv", 0)) << "segment 1 must be kept";
    this->off();
}

void OpticalCameraTester ::testOffWhenIdle() {
    this->off();
    ASSERT_EVENTS_SIZE(0);
}

void OpticalCameraTester ::testExitedEarly() {
    this->setMode("exit1");
    this->on();
    ASSERT_EVENTS_RecordingStarted_SIZE(1);
    sleepMs(500);  // let it exit

    this->ping();
    ASSERT_from_pingOut_SIZE(1);  // always answered
    ASSERT_EVENTS_RecorderExitedEarly_SIZE(1);
    ASSERT_EQ(this->eventHistory_RecorderExitedEarly->at(0).seg, 1U);

    this->ping();
    ASSERT_EVENTS_RecorderExitedEarly_SIZE(1);  // reported once

    // Not recording any more, so ON starts a new segment
    this->setMode("record");
    this->startRecording();
    ASSERT_EVENTS_RecordingStarted_SIZE(2);
    this->off();
}

void OpticalCameraTester ::testExitedNormally() {
    this->setMode("exit0");
    this->on();
    sleepMs(500);

    this->ping();
    ASSERT_EVENTS_RecorderExitedEarly_SIZE(0);
    this->off();  // nothing left to stop
    ASSERT_EVENTS_RecordingStopped_SIZE(0);
}

void OpticalCameraTester ::testStall() {
    this->setMode("stall");
    this->startRecording();

    // First ping sees the file grow from 0; then 5 pings without growth
    this->ping();
    for (U32 i = 0; i < 4; i++) {
        this->ping();
    }
    ASSERT_EVENTS_RecorderStalled_SIZE(0);
    this->ping();
    ASSERT_EVENTS_RecorderStalled_SIZE(1);
    ASSERT_EQ(this->eventHistory_RecorderStalled->at(0).seg, 1U);
    ASSERT_GT(this->eventHistory_RecorderStalled->at(0).bytes, 0U);

    for (U32 i = 0; i < 5; i++) {
        this->ping();
    }
    ASSERT_EVENTS_RecorderStalled_SIZE(1);  // reported once
    ASSERT_from_pingOut_SIZE(11);
    this->off();
}

void OpticalCameraTester ::testNoStallWhileGrowing() {
    this->startRecording();
    for (U32 i = 0; i < 10; i++) {
        sleepMs(150);  // the fake writes every 50 ms
        this->ping();
    }
    ASSERT_EVENTS_RecorderStalled_SIZE(0);
    this->off();
}

void OpticalCameraTester ::testSpawnFailed() {
    // Nothing called ffmpeg on PATH
    const std::string emptyDir = m_dir + "/empty";
    ASSERT_EQ(::mkdir(emptyDir.c_str(), 0755), 0);
    ASSERT_EQ(::setenv("PATH", emptyDir.c_str(), 1), 0);

    this->on();
    ASSERT_EVENTS_SpawnFailed_SIZE(1);
    ASSERT_EVENTS_RecordingStarted_SIZE(0);
    this->ping();
    ASSERT_from_pingOut_SIZE(1);

    // Fix PATH: the component keeps working
    ASSERT_EQ(::setenv("PATH", (m_dir + "/bin:/usr/bin:/bin").c_str(), 1), 0);
    this->startRecording();
    ASSERT_EVENTS_RecordingStarted_SIZE(1);
    this->off();
}

// ----------------------------------------------------------------------
// Helper functions
// ----------------------------------------------------------------------

void OpticalCameraTester ::on() {
    this->invoke_to_Camera_ON(0);
    this->component.doDispatch();
}

void OpticalCameraTester ::off() {
    this->invoke_to_Camera_OFF(0);
    this->component.doDispatch();
}

void OpticalCameraTester ::ping() {
    this->invoke_to_pingIn(0, 0);
    this->component.doDispatch();
}

void OpticalCameraTester ::startRecording() {
    const FwSizeType before = this->eventHistory_RecordingStarted->size();
    this->on();
    ASSERT_EQ(this->eventHistory_RecordingStarted->size(), before + 1);
    const U32 segment = this->eventHistory_RecordingStarted->at(before).seg;
    std::ostringstream name;
    name << "/cam_" << std::setw(3) << std::setfill('0') << segment << ".mkv";
    ASSERT_TRUE(this->waitForFile(m_primary + name.str(), 2000)) << "fake ffmpeg did not start writing";
}

void OpticalCameraTester ::setMode(const char* mode) {
    ASSERT_EQ(::setenv("FAKE_FFMPEG_MODE", mode, 1), 0);
}

std::vector<std::string> OpticalCameraTester ::logLines() const {
    std::vector<std::string> lines;
    std::ifstream file(this->logPath());
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}

std::vector<std::string> OpticalCameraTester ::lastArgs() const {
    std::vector<std::string> args;
    std::ifstream file(this->logPath() + ".args");
    std::string line;
    while (std::getline(file, line)) {
        args.push_back(line);
    }
    return args;
}

bool OpticalCameraTester ::waitForFile(const std::string& path, U32 timeoutMs) const {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (true) {
        struct stat info;
        if (::stat(path.c_str(), &info) == 0 && info.st_size > 0) {
            return true;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
        sleepMs(20);
    }
}

void OpticalCameraTester ::sleepMs(U32 ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

}  // namespace Mara
