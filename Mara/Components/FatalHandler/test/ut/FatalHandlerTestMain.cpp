// ======================================================================
// \title  FatalHandlerTestMain.cpp
// \author fsowa
// \brief  cpp file for FatalHandler component test main function
// ======================================================================

#include "FatalHandlerTester.hpp"

TEST(Fatal, Survived) {
    Mara::FatalHandlerTester tester;
    tester.testFatalIsSurvived();
}

TEST(Fatal, ThrottledButCounted) {
    Mara::FatalHandlerTester tester;
    tester.testThrottledButCounted();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
