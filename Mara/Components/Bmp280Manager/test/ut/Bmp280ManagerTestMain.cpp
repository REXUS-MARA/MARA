// ======================================================================
// \title  Bmp280ManagerTestMain.cpp
// \author fsowa
// \brief  cpp file for Bmp280Manager component test main function
// ======================================================================

#include "Bmp280ManagerTester.hpp"

TEST(Startup, Sequence) {
    Mara::Bmp280ManagerTester tester;
    tester.testStartupSequence();
}

TEST(Startup, WaitsForNvmCopy) {
    Mara::Bmp280ManagerTester tester;
    tester.testWaitsForNvmCopy();
}

TEST(Startup, WrongChipId) {
    Mara::Bmp280ManagerTester tester;
    tester.testWrongChipId();
}

TEST(Reading, DatasheetCompensation) {
    Mara::Bmp280ManagerTester tester;
    tester.testDatasheetCompensation();
}

TEST(Reading, LowPressureClamp) {
    Mara::Bmp280ManagerTester tester;
    tester.testLowPressureClamp();
}

TEST(Config, Registers) {
    Mara::Bmp280ManagerTester tester;
    tester.testConfigRegisters();
}

TEST(Config, ReconfigureOnParameter) {
    Mara::Bmp280ManagerTester tester;
    tester.testReconfigureOnParameter();
}

TEST(Recovery, BusError) {
    Mara::Bmp280ManagerTester tester;
    tester.testBusErrorRecovery();
}

TEST(Recovery, ResetCommand) {
    Mara::Bmp280ManagerTester tester;
    tester.testResetCommand();
}

TEST(DataProduct, FullContainer) {
    Mara::Bmp280ManagerTester tester;
    tester.testDataProduct();
}

TEST(DataProduct, ReconfigureClosesContainer) {
    Mara::Bmp280ManagerTester tester;
    tester.testReconfigureClosesContainer();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
