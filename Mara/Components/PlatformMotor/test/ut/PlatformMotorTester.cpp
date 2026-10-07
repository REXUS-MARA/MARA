// ======================================================================
// \title  PlatformMotorTester.cpp
// \author fsowa
// \brief  cpp file for PlatformMotor component test harness implementation class
// ======================================================================

#include "PlatformMotorTester.hpp"

#include <algorithm>

namespace Mara {

namespace {
// CiA 402 objects and controlwords, as the drive expects them
constexpr U16 CONTROLWORD = 0x6040;
constexpr U16 MODE_OF_OPERATION = 0x6060;
constexpr U16 TARGET_POSITION = 0x607A;
constexpr U16 PROFILE_VELOCITY = 0x6081;
constexpr U16 PROFILE_ACCELERATION = 0x6083;

constexpr U32 CW_FAULT_RESET = 0x0080;
constexpr U32 CW_SHUTDOWN = 0x0006;
constexpr U32 CW_SWITCH_ON = 0x0007;
constexpr U32 CW_ENABLE = 0x000F;
constexpr U32 CW_START_MOVE = 0x003F;
constexpr U32 CW_HALT = 0x010F;
}  // namespace

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

PlatformMotorTester ::PlatformMotorTester()
    : PlatformMotorGTestBase("PlatformMotorTester", PlatformMotorTester::MAX_HISTORY_SIZE), component("PlatformMotor") {
    this->initComponents();
    this->connectPorts();
}

PlatformMotorTester ::~PlatformMotorTester() {}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void PlatformMotorTester ::testEnableFrames() {
    this->setParams(0, 1000);
    this->enable();

    ASSERT_EQ(m_frames.size(), 5U);
    // Byte-exact first frame: mode of operation = 1 (profile position), 1-byte SDO, node 1
    const Frame expected{0xAA, 0xC8, 0x01, 0x06, 0x2F, 0x60, 0x60, 0x00, 0x01, 0x00, 0x00, 0x00, 0x55};
    ASSERT_EQ(m_frames[0], expected);
    this->assertSdo(0, MODE_OF_OPERATION, 1, 1);
    this->assertSdo(1, CONTROLWORD, CW_FAULT_RESET, 2);
    this->assertSdo(2, CONTROLWORD, CW_SHUTDOWN, 2);
    this->assertSdo(3, CONTROLWORD, CW_SWITCH_ON, 2);
    this->assertSdo(4, CONTROLWORD, CW_ENABLE, 2);

    ASSERT_EVENTS_Enabled_SIZE(1);
    ASSERT_EVENTS_LimitsUnset_SIZE(0);
    ASSERT_EVENTS_SendFailed_SIZE(0);
}

void PlatformMotorTester ::testEnableWithAcceleration() {
    this->setParams(0, 1000, 0, 0, 500);
    this->enable();

    ASSERT_EQ(m_frames.size(), 6U);
    this->assertSdo(0, MODE_OF_OPERATION, 1, 1);
    this->assertSdo(1, PROFILE_ACCELERATION, 500, 4);
    this->assertSdo(5, CONTROLWORD, CW_ENABLE, 2);
    ASSERT_EVENTS_Enabled_SIZE(1);
}

void PlatformMotorTester ::testEnableLimitsUnset() {
    this->setParams(0, 0);
    this->enable();

    ASSERT_EVENTS_LimitsUnset_SIZE(1);
    ASSERT_EQ(m_frames.size(), 5U);  // the enable still happens
    ASSERT_EVENTS_Enabled_SIZE(1);
}

void PlatformMotorTester ::testNodeId() {
    this->setParams(0, 1000, 0, 0, 0, 0x7F);
    this->enable();
    this->moveTo(10);
    this->stop();

    ASSERT_FALSE(m_frames.empty());
    for (FwSizeType i = 0; i < m_frames.size(); i++) {
        ASSERT_EQ(this->decode(i).cobId, 0x67F) << "frame " << i;
    }
}

void PlatformMotorTester ::testMoveFrames() {
    this->setParams(0, 1000);
    this->moveTo(500);

    ASSERT_EQ(m_frames.size(), 3U);  // no velocity write: both velocities are 0
    this->assertSdo(0, CONTROLWORD, CW_ENABLE, 2);
    this->assertSdo(1, TARGET_POSITION, 500, 4);
    this->assertSdo(2, CONTROLWORD, CW_START_MOVE, 2);

    ASSERT_EVENTS_MovingTo_SIZE(1);
    ASSERT_EVENTS_MovingTo(0, 500);
    ASSERT_EVENTS_PositionClamped_SIZE(0);
    ASSERT_TLM_TargetPosition_SIZE(1);
    ASSERT_TLM_TargetPosition(0, 500);
}

void PlatformMotorTester ::testVelocityPositiveConvention() {
    this->setParams(0, 1000, 111, 222);

    this->moveTo(800);  // away from 0: advance
    ASSERT_EQ(m_frames.size(), 4U);
    this->assertSdo(0, PROFILE_VELOCITY, 111, 4);

    this->clearAll();
    this->moveTo(300);  // closer to 0: retract
    this->assertSdo(0, PROFILE_VELOCITY, 222, 4);

    this->clearAll();
    this->moveTo(0);  // to 0: retract
    this->assertSdo(0, PROFILE_VELOCITY, 222, 4);
    this->assertSdo(2, TARGET_POSITION, 0, 4);
}

void PlatformMotorTester ::testVelocityNegativeConvention() {
    // The drive counts negative towards the sample
    this->setParams(-2000, 0, 111, 222);

    this->moveTo(-1000);  // away from 0: advance
    this->assertSdo(0, PROFILE_VELOCITY, 111, 4);
    this->assertSdo(2, TARGET_POSITION, static_cast<U32>(-1000), 4);
    ASSERT_EVENTS_PositionClamped_SIZE(0);

    this->clearAll();
    this->moveTo(0);  // back to 0: retract
    this->assertSdo(0, PROFILE_VELOCITY, 222, 4);
}

void PlatformMotorTester ::testClamping() {
    this->setParams(-100, 1000);

    this->moveTo(5000);
    this->assertSdo(1, TARGET_POSITION, 1000, 4);
    ASSERT_EVENTS_PositionClamped_SIZE(1);
    ASSERT_EVENTS_PositionClamped(0, 5000, 1000);
    ASSERT_EVENTS_MovingTo(0, 1000);

    this->clearAll();
    this->moveTo(-500);
    this->assertSdo(1, TARGET_POSITION, static_cast<U32>(-100), 4);
    ASSERT_EVENTS_PositionClamped(0, -500, -100);

    this->clearAll();
    this->moveTo(1000);  // exactly at the limit: not clamped
    ASSERT_EVENTS_PositionClamped_SIZE(0);
    ASSERT_EVENTS_MovingTo(0, 1000);
}

void PlatformMotorTester ::testClampingLimitsUnset() {
    this->setParams(0, 0);
    this->moveTo(500);

    this->assertSdo(1, TARGET_POSITION, 0, 4);
    ASSERT_EVENTS_PositionClamped(0, 500, 0);
    ASSERT_EVENTS_MovingTo(0, 0);
}

void PlatformMotorTester ::testNegativeTargetEncoding() {
    this->setParams(-20000, 0);
    this->moveTo(-12345);

    // -12345 = 0xFFFFCFC7, little-endian in data bytes 4..7 of the SDO
    const Frame& frame = m_frames.at(1);
    ASSERT_EQ(frame[8], 0xC7);
    ASSERT_EQ(frame[9], 0xCF);
    ASSERT_EQ(frame[10], 0xFF);
    ASSERT_EQ(frame[11], 0xFF);
}

void PlatformMotorTester ::testStopFrame() {
    this->setParams(0, 1000);
    this->stop();

    ASSERT_EQ(m_frames.size(), 1U);
    this->assertSdo(0, CONTROLWORD, CW_HALT, 2);
    ASSERT_EQ(m_frames[0][8], 0x0F);  // 0x010F little-endian
    ASSERT_EQ(m_frames[0][9], 0x01);
    ASSERT_EVENTS_Stopped_SIZE(1);
}

void PlatformMotorTester ::testSendFailureMidMove() {
    this->setParams(0, 1000, 111, 222);

    // Fail the second write of the move (0x0F after the velocity)
    m_failSendIndex = 1;
    this->moveTo(800);

    ASSERT_EQ(m_sendAttempts, 2U);  // nothing sent after the failure
    ASSERT_EVENTS_SendFailed_SIZE(1);
    ASSERT_EVENTS_SendFailed(0, Drv::ByteStreamStatus::OTHER_ERROR);
    ASSERT_EVENTS_MovingTo_SIZE(0);
    ASSERT_TLM_TargetPosition_SIZE(0);

    // The last target is still 0: a move to 400 is away from 0, so it uses ADVANCE.
    // (Had the failed move updated the target to 800, it would use RETRACT.)
    m_failSendIndex = -1;
    this->clearAll();
    this->moveTo(400);
    this->assertSdo(0, PROFILE_VELOCITY, 111, 4);
    ASSERT_EVENTS_SendFailed_SIZE(0);
    ASSERT_EVENTS_MovingTo(0, 400);
}

void PlatformMotorTester ::testSendFailureEnable() {
    this->setParams(0, 1000);
    m_failSendIndex = 0;
    this->enable();

    ASSERT_EQ(m_sendAttempts, 1U);
    ASSERT_EVENTS_SendFailed_SIZE(1);
    ASSERT_EVENTS_Enabled_SIZE(0);

    // Fail in the controlword part instead
    this->clearAll();
    m_failSendIndex = 3;
    this->enable();
    ASSERT_EQ(m_sendAttempts, 4U);
    ASSERT_EVENTS_SendFailed_SIZE(1);
    ASSERT_EVENTS_Enabled_SIZE(0);
}

void PlatformMotorTester ::testSendFailureStop() {
    this->setParams(0, 1000);
    m_failSendIndex = 0;
    this->stop();

    ASSERT_EVENTS_SendFailed_SIZE(1);
    ASSERT_EVENTS_Stopped_SIZE(0);
}

void PlatformMotorTester ::testSendFailureAcceleration() {
    this->setParams(0, 1000, 0, 0, 500);
    m_failSendIndex = 1;  // mode, then acceleration
    this->enable();

    ASSERT_EQ(m_sendAttempts, 2U);
    ASSERT_EVENTS_SendFailed_SIZE(1);
    ASSERT_EVENTS_Enabled_SIZE(0);
}

void PlatformMotorTester ::testSendFailureVelocity() {
    this->setParams(0, 1000, 111, 222);
    m_failSendIndex = 0;  // the velocity is the first write of the move
    this->moveTo(500);

    ASSERT_EQ(m_sendAttempts, 1U);
    ASSERT_EVENTS_SendFailed_SIZE(1);
    ASSERT_EVENTS_MovingTo_SIZE(0);
    ASSERT_TLM_TargetPosition_SIZE(0);
}

void PlatformMotorTester ::testDriverReady() {
    this->invoke_to_byteStreamDriverReady(0);  // sync port
    ASSERT_EQ(m_sendAttempts, 0U);
    ASSERT_EVENTS_SIZE(0);
}

void PlatformMotorTester ::testPing() {
    this->invoke_to_pingIn(0, 0x1234);
    this->component.doDispatch();

    ASSERT_from_pingOut_SIZE(1);
    ASSERT_from_pingOut(0, 0x1234);
}

void PlatformMotorTester ::testReceiveReturnsBuffer() {
    std::array<U8, 13> data{};
    Fw::Buffer buffer(data.data(), data.size());

    // Sync port: handled on the caller's thread, no dispatch
    this->invoke_to_fromByteStreamDriver(0, buffer, Drv::ByteStreamStatus::OP_OK);

    ASSERT_from_fromByteStreamDriverReturn_SIZE(1);
    ASSERT_EQ(this->fromPortHistory_fromByteStreamDriverReturn->at(0).fwBuffer.getData(), data.data());
    ASSERT_EQ(m_frames.size(), 0U);  // nothing sent in reply
}

void PlatformMotorTester ::testQueueOverflowDrops() {
    this->setParams(0, 1000);
    PlatformMotorComponentBase& base = this->component;
    const FwSizeType depth = TEST_INSTANCE_QUEUE_DEPTH;  // local copy: gtest binds by reference

    // More messages than the queue holds, without dispatching: must not assert
    const FwSizeType sent = depth + 5;
    for (FwSizeType i = 0; i < sent; i++) {
        this->invoke_to_moveTo(0, static_cast<I32>(i));
    }
    ASSERT_EQ(base.m_queue.getMessagesAvailable(), depth);
    ASSERT_EQ(base.getNumMsgsDropped(), sent - depth);

    // The queued ones are processed normally
    for (FwSizeType i = 0; i < depth; i++) {
        this->component.doDispatch();
    }
    ASSERT_EVENTS_MovingTo_SIZE(depth);
    ASSERT_EVENTS_MovingTo(depth - 1, static_cast<I32>(depth - 1));
}

// ----------------------------------------------------------------------
// Handlers for typed from ports
// ----------------------------------------------------------------------

Drv::ByteStreamStatus PlatformMotorTester ::from_toByteStreamDriver_handler(FwIndexType portNum,
                                                                          Fw::Buffer& sendBuffer) {
    (void)portNum;
    const FwSizeType attempt = m_sendAttempts++;
    if (m_failSendIndex >= 0 && attempt == static_cast<FwSizeType>(m_failSendIndex)) {
        return Drv::ByteStreamStatus::OTHER_ERROR;
    }

    EXPECT_EQ(sendBuffer.getSize(), 13U);
    Frame frame{};
    std::copy_n(sendBuffer.getData(), std::min<FwSizeType>(sendBuffer.getSize(), frame.size()), frame.begin());
    m_frames.push_back(frame);
    return Drv::ByteStreamStatus::OP_OK;
}

// ----------------------------------------------------------------------
// Helper functions
// ----------------------------------------------------------------------

void PlatformMotorTester ::setParams(I32 minPosition,
                                     I32 maxPosition,
                                     U32 advanceVelocity,
                                     U32 retractVelocity,
                                     U32 acceleration,
                                     U8 nodeId) {
    this->paramSet_MIN_POSITION(minPosition, Fw::ParamValid::VALID);
    this->paramSet_MAX_POSITION(maxPosition, Fw::ParamValid::VALID);
    this->paramSet_ADVANCE_VELOCITY(advanceVelocity, Fw::ParamValid::VALID);
    this->paramSet_RETRACT_VELOCITY(retractVelocity, Fw::ParamValid::VALID);
    this->paramSet_ACCELERATION(acceleration, Fw::ParamValid::VALID);
    this->paramSet_NODE_ID(nodeId, Fw::ParamValid::VALID);
    this->component.loadParameters();
}

void PlatformMotorTester ::enable() {
    this->invoke_to_enable(0);
    this->component.doDispatch();
}

void PlatformMotorTester ::moveTo(I32 position) {
    this->invoke_to_moveTo(0, position);
    this->component.doDispatch();
}

void PlatformMotorTester ::stop() {
    this->invoke_to_stop(0);
    this->component.doDispatch();
}

PlatformMotorTester::Sdo PlatformMotorTester ::decode(FwSizeType frameIndex) const {
    const Frame& f = m_frames.at(frameIndex);
    EXPECT_EQ(f[0], 0xAA) << "frame " << frameIndex << ": header";
    EXPECT_EQ(f[1], 0xC8) << "frame " << frameIndex << ": type (standard data frame, 8 bytes)";
    EXPECT_EQ(f[12], 0x55) << "frame " << frameIndex << ": end code";
    Sdo sdo{};
    sdo.cobId = static_cast<U16>(f[2] | (f[3] << 8));
    sdo.command = f[4];
    sdo.index = static_cast<U16>(f[5] | (f[6] << 8));
    sdo.subindex = f[7];
    sdo.value = static_cast<U32>(f[8]) | (static_cast<U32>(f[9]) << 8) | (static_cast<U32>(f[10]) << 16) |
                (static_cast<U32>(f[11]) << 24);
    return sdo;
}

void PlatformMotorTester ::assertSdo(FwSizeType frameIndex, U16 index, U32 value, U8 size, U8 nodeId) const {
    ASSERT_LT(frameIndex, m_frames.size());
    const Sdo sdo = this->decode(frameIndex);
    const U8 command = (size == 4) ? 0x23 : (size == 2) ? 0x2B : 0x2F;
    EXPECT_EQ(sdo.cobId, 0x600 + nodeId) << "frame " << frameIndex;
    EXPECT_EQ(sdo.command, command) << "frame " << frameIndex;
    EXPECT_EQ(sdo.index, index) << "frame " << frameIndex;
    EXPECT_EQ(sdo.subindex, 0) << "frame " << frameIndex;
    EXPECT_EQ(sdo.value, value) << "frame " << frameIndex;
}

void PlatformMotorTester ::clearAll() {
    m_frames.clear();
    m_sendAttempts = 0;
    this->clearHistory();
}

}  // namespace Mara
