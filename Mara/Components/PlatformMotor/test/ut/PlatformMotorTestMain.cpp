// ======================================================================
// \title  PlatformMotorTestMain.cpp
// \author fsowa
// \brief  cpp file for PlatformMotor component test main function
// ======================================================================

#include "PlatformMotorTester.hpp"

TEST(Enable, Frames) {
    Mara::PlatformMotorTester tester;
    tester.testEnableFrames();
}

TEST(Enable, WithAcceleration) {
    Mara::PlatformMotorTester tester;
    tester.testEnableWithAcceleration();
}

TEST(Enable, LimitsUnset) {
    Mara::PlatformMotorTester tester;
    tester.testEnableLimitsUnset();
}

TEST(Frames, NodeId) {
    Mara::PlatformMotorTester tester;
    tester.testNodeId();
}

TEST(Move, Frames) {
    Mara::PlatformMotorTester tester;
    tester.testMoveFrames();
}

TEST(Move, VelocityPositiveConvention) {
    Mara::PlatformMotorTester tester;
    tester.testVelocityPositiveConvention();
}

TEST(Move, VelocityNegativeConvention) {
    Mara::PlatformMotorTester tester;
    tester.testVelocityNegativeConvention();
}

TEST(Move, Clamping) {
    Mara::PlatformMotorTester tester;
    tester.testClamping();
}

TEST(Move, ClampingLimitsUnset) {
    Mara::PlatformMotorTester tester;
    tester.testClampingLimitsUnset();
}

TEST(Move, NegativeTargetEncoding) {
    Mara::PlatformMotorTester tester;
    tester.testNegativeTargetEncoding();
}

TEST(Stop, Frame) {
    Mara::PlatformMotorTester tester;
    tester.testStopFrame();
}

TEST(SendFailure, MidMove) {
    Mara::PlatformMotorTester tester;
    tester.testSendFailureMidMove();
}

TEST(SendFailure, Enable) {
    Mara::PlatformMotorTester tester;
    tester.testSendFailureEnable();
}

TEST(SendFailure, Stop) {
    Mara::PlatformMotorTester tester;
    tester.testSendFailureStop();
}

TEST(SendFailure, Acceleration) {
    Mara::PlatformMotorTester tester;
    tester.testSendFailureAcceleration();
}

TEST(SendFailure, Velocity) {
    Mara::PlatformMotorTester tester;
    tester.testSendFailureVelocity();
}

TEST(Ports, DriverReady) {
    Mara::PlatformMotorTester tester;
    tester.testDriverReady();
}

TEST(Ports, Ping) {
    Mara::PlatformMotorTester tester;
    tester.testPing();
}

TEST(Ports, ReceiveReturnsBuffer) {
    Mara::PlatformMotorTester tester;
    tester.testReceiveReturnsBuffer();
}

TEST(Queue, OverflowDrops) {
    Mara::PlatformMotorTester tester;
    tester.testQueueOverflowDrops();
}

TEST(Negative, InvertedLimitsRefuseToMove) {
    Mara::PlatformMotorTester tester;
    tester.testInvertedLimitsRefuseToMove();
}

TEST(Negative, ZeroOutsideLimitsWarns) {
    Mara::PlatformMotorTester tester;
    tester.testZeroOutsideLimitsWarns();
}

TEST(Negative, InvalidNodeIdSendsNothing) {
    Mara::PlatformMotorTester tester;
    tester.testInvalidNodeIdSendsNothing();
}

TEST(Negative, ExtremeTargetsClamped) {
    Mara::PlatformMotorTester tester;
    tester.testExtremeTargetsClamped();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
