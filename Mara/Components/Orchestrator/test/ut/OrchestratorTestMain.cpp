// ======================================================================
// \title  OrchestratorTestMain.cpp
// \author fsowa
// \brief  cpp file for Orchestrator component test main function
// ======================================================================

#include "OrchestratorTester.hpp"

TEST(Flight, Path) {
    Mara::OrchestratorTester tester;
    tester.testFlightPath();
}

TEST(Timeline, Timing) {
    Mara::OrchestratorTester tester;
    tester.testTimelineTiming();
}

TEST(Timeline, ParamsLoadedOnce) {
    Mara::OrchestratorTester tester;
    tester.testTimelineParamsLoadedOnce();
}

TEST(Timeline, DrillPositionUnset) {
    Mara::OrchestratorTester tester;
    tester.testDrillPositionUnset();
}

TEST(Retract, EodsDuringSpinUp) {
    Mara::OrchestratorTester tester;
    tester.testEodsDuringSpinUpRetracts();
}

TEST(Retract, EodsDuringAdvance) {
    Mara::OrchestratorTester tester;
    tester.testEodsDuringAdvanceRetracts();
}

TEST(Retract, EodsAfterDrillingNoSecondRetract) {
    Mara::OrchestratorTester tester;
    tester.testEodsAfterDrillingNoSecondRetract();
}

TEST(Retract, ErrorWhileDrilling) {
    Mara::OrchestratorTester tester;
    tester.testErrorWhileDrillingRetracts();
}

TEST(TestMode, CommandsSignalsAndExit) {
    Mara::OrchestratorTester tester;
    tester.testTestMode();
}

TEST(TestMode, CommandGating) {
    Mara::OrchestratorTester tester;
    tester.testCommandGating();
}

TEST(TestMode, EnterExitGating) {
    Mara::OrchestratorTester tester;
    tester.testEnterExitGating();
}

TEST(Ticks, OutsideExperiment) {
    Mara::OrchestratorTester tester;
    tester.testTicksOutsideExperiment();
}

TEST(Safe, ErrorToSafe) {
    Mara::OrchestratorTester tester;
    tester.testErrorToSafe();
}

TEST(Queue, OverflowDrops) {
    Mara::OrchestratorTester tester;
    tester.testQueueOverflowDrops();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
