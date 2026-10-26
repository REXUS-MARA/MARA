// ======================================================================
// \title  ReconnectingUartDriverTestMain.cpp
// \author fsowa
// \brief  cpp file for ReconnectingUartDriver component test main function
// ======================================================================

#include "ReconnectingUartDriverTester.hpp"

#include <tuple>

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

TEST(Settings, NotASerialPort) {
    Mara::ReconnectingUartDriverTester tester;
    tester.testNotASerialPort();
}

TEST(Settings, UnsupportedBaud) {
    Mara::ReconnectingUartDriverTester tester;
    tester.testUnsupportedBaud();
}

TEST(Send, BufferFull) {
    Mara::ReconnectingUartDriverTester tester;
    tester.testSendBufferFull();
}

using Driver = Mara::ReconnectingUartDriver;
using Setting = std::tuple<Driver::UartBaudRate, Driver::UartParity, Driver::UartFlowControl>;

class AllSettings : public ::testing::TestWithParam<Setting> {};

TEST_P(AllSettings, Connect) {
    Mara::ReconnectingUartDriverTester tester;
    tester.testAllSettingsConnect(std::get<0>(GetParam()), std::get<1>(GetParam()), std::get<2>(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(
    Settings,
    AllSettings,
    ::testing::Values(Setting{Driver::BAUD_9600, Driver::PARITY_NONE, Driver::NO_FLOW},
                      Setting{Driver::BAUD_19200, Driver::PARITY_EVEN, Driver::NO_FLOW},
                      Setting{Driver::BAUD_38400, Driver::PARITY_ODD, Driver::NO_FLOW},
                      Setting{Driver::BAUD_57600, Driver::PARITY_NONE, Driver::HW_FLOW},
                      Setting{Driver::BAUD_115K, Driver::PARITY_NONE, Driver::NO_FLOW},
                      Setting{Driver::BAUD_230K, Driver::PARITY_NONE, Driver::NO_FLOW}
#ifdef TGT_OS_TYPE_LINUX
                      ,
                      Setting{Driver::BAUD_460K, Driver::PARITY_NONE, Driver::NO_FLOW},
                      Setting{Driver::BAUD_921K, Driver::PARITY_NONE, Driver::NO_FLOW},
                      Setting{Driver::BAUD_1000K, Driver::PARITY_NONE, Driver::NO_FLOW},
                      Setting{Driver::BAUD_1500K, Driver::PARITY_NONE, Driver::NO_FLOW},
                      Setting{Driver::BAUD_2000K, Driver::PARITY_NONE, Driver::NO_FLOW}
#endif
                      ));

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
