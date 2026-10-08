// ======================================================================
// \title  OrchestratorTestMain.cpp
// \author fsowa
// \brief  cpp file for Orchestrator component test main function
// ======================================================================

#include "OrchestratorRules.hpp"
#include "OrchestratorTester.hpp"
#include "STest/Random/Random.hpp"

#include <cstdio>

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

// Random inputs (STest rules) on fresh components with random timelines; every step is
// checked against the safety invariants in OrchestratorTester::checkStep().
// The seed comes from the file "seed" if present (replay), else from the clock, and is
// appended to "seed-history".
TEST(Random, SafetyInvariants) {
    const U32 steps = Mara::runRandomScenarios(300, 60);
    std::printf("Ran %u random steps.\n", steps);
    ASSERT_GT(steps, 0U);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
