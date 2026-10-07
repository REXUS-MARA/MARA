// ======================================================================
// \title  ReconnectingUartDriverTestMain.cpp
// \author fsowa
// \brief  cpp file for ReconnectingUartDriver component test main function
// ======================================================================

#include "ReconnectingUartDriverTester.hpp"

TEST(Connection, NotConnectedAtStart) {
    Mara::ReconnectingUartDriverTester tester;
    tester.testNotConnectedAtStart();
}

TEST(Connection, ConnectReceiveSend) {
    Mara::ReconnectingUartDriverTester tester;
    tester.testConnectReceiveSend();
}

TEST(Connection, UnplugReplugCycles) {
    Mara::ReconnectingUartDriverTester tester;
    tester.testUnplugReplugCycles();
}

TEST(Buffers, NoBuffers) {
    Mara::ReconnectingUartDriverTester tester;
    tester.testNoBuffers();
}

TEST(Shutdown, Disconnected) {
    Mara::ReconnectingUartDriverTester tester;
    tester.testShutdown(false);
}

TEST(Shutdown, Connected) {
    Mara::ReconnectingUartDriverTester tester;
    tester.testShutdown(true);
}

TEST(Send, Empty) {
    Mara::ReconnectingUartDriverTester tester;
    tester.testEmptySend();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
