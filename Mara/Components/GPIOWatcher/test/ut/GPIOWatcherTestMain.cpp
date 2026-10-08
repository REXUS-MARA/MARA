// ======================================================================
// \title  GPIOWatcherTestMain.cpp
// \author fsowa
// \brief  cpp file for GPIOWatcher component test main function
// ======================================================================

#include "GPIOWatcherTester.hpp"

using Line = Mara::GPIOWatcherTester::Line;

class PerLine : public ::testing::TestWithParam<Line> {};

TEST_P(PerLine, Detect) {
    Mara::GPIOWatcherTester tester;
    tester.testDetect(GetParam());
}

TEST_P(PerLine, Spike) {
    Mara::GPIOWatcherTester tester;
    tester.testSpike(GetParam());
}

TEST_P(PerLine, Refire) {
    Mara::GPIOWatcherTester tester;
    tester.testRefire(GetParam());
}

INSTANTIATE_TEST_SUITE_P(Lines,
                         PerLine,
                         ::testing::Values(Mara::GPIOWatcherTester::LO,
                                           Mara::GPIOWatcherTester::SOE,
                                           Mara::GPIOWatcherTester::EODS));

TEST(Lines, Independent) {
    Mara::GPIOWatcherTester tester;
    tester.testIndependentLines();
}

TEST(Lines, ReadFailureNeverFires) {
    Mara::GPIOWatcherTester tester;
    tester.testReadFailureNeverFires();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
