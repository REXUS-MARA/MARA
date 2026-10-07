// ======================================================================
// \title  OpticalCameraTestMain.cpp
// \author fsowa
// \brief  cpp file for OpticalCamera component test main function
// ======================================================================

#include "OpticalCameraTester.hpp"

#include <fstream>
#include <memory>
#include <string>

TEST(Recording, OnOffArguments) {
    Mara::OpticalCameraTester tester;
    tester.testOnOffArguments();
}

TEST(Recording, OnIdempotentAndSegments) {
    Mara::OpticalCameraTester tester;
    tester.testOnIdempotentAndSegments();
}

TEST(Recording, OffWhenIdle) {
    Mara::OpticalCameraTester tester;
    tester.testOffWhenIdle();
}

TEST(Monitoring, ExitedEarly) {
    Mara::OpticalCameraTester tester;
    tester.testExitedEarly();
}

TEST(Monitoring, ExitedNormally) {
    Mara::OpticalCameraTester tester;
    tester.testExitedNormally();
}

TEST(Monitoring, Stall) {
    Mara::OpticalCameraTester tester;
    tester.testStall();
}

TEST(Monitoring, NoStallWhileGrowing) {
    Mara::OpticalCameraTester tester;
    tester.testNoStallWhileGrowing();
}

TEST(Recording, SpawnFailed) {
    Mara::OpticalCameraTester tester;
    tester.testSpawnFailed();
}

// Shutting the flight software down while recording must not leave ffmpeg running:
// the component's destructor sends SIGINT and waits.
TEST(Recording, DestructorStopsRecorder) {
    auto tester = std::make_unique<Mara::OpticalCameraTester>();
    const std::string log = tester->logPath();
    tester->startRecording();
    tester.reset();

    std::ifstream file(log);
    std::string line;
    bool sawSigint = false;
    while (std::getline(file, line)) {
        sawSigint = sawSigint || (line == "sigint");
    }
    ASSERT_TRUE(sawSigint);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
