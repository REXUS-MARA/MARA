// ======================================================================
// \title  PlatformMotorTester.hpp
// \author fsowa
// \brief  hpp file for PlatformMotor component test harness implementation class
// ======================================================================

#ifndef Mara_PlatformMotorTester_HPP
#define Mara_PlatformMotorTester_HPP

#include "Mara/Components/PlatformMotor/PlatformMotor.hpp"
#include "Mara/Components/PlatformMotor/PlatformMotorGTestBase.hpp"

#include <array>
#include <vector>

namespace Mara {

class PlatformMotorTester final : public PlatformMotorGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 100;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    // Queue depth supplied to the component instance under test
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

    //! One Waveshare USB-CAN-A frame as sent to the driver
    using Frame = std::array<U8, 13>;

    //! The CANopen SDO download carried by a frame
    struct Sdo {
        U16 cobId;
        U8 command;
        U16 index;
        U8 subindex;
        U32 value;
    };

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object PlatformMotorTester
    PlatformMotorTester();

    //! Destroy object PlatformMotorTester
    ~PlatformMotorTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! Enable sends mode, then the CiA 402 state sequence, as byte-exact frames
    void testEnableFrames();

    //! A non-zero ACCELERATION is written on enable
    void testEnableWithAcceleration();

    //! MIN/MAX both 0 raises LimitsUnset but still enables
    void testEnableLimitsUnset();

    //! NODE_ID selects the SDO COB-ID
    void testNodeId();

    //! moveTo sends 0x0F, the target and 0x3F
    void testMoveFrames();

    //! Velocity is chosen by distance from 0, with a positive "up" convention
    void testVelocityPositiveConvention();

    //! Velocity is chosen by distance from 0, with a negative "up" convention
    void testVelocityNegativeConvention();

    //! Targets are clamped to [MIN, MAX] with PositionClamped
    void testClamping();

    //! MIN = MAX = 0 clamps every move to 0
    void testClampingLimitsUnset();

    //! Negative targets are encoded two's-complement little-endian
    void testNegativeTargetEncoding();

    //! stop sends the halt controlword
    void testStopFrame();

    //! A failed send mid-command logs one SendFailed and abandons the command
    void testSendFailureMidMove();

    //! A failed send during enable abandons the enable
    void testSendFailureEnable();

    //! A failed send during stop does not report Stopped
    void testSendFailureStop();

    //! Pings are answered
    void testPing();

    //! Received buffers are returned to the driver
    void testReceiveReturnsBuffer();

    //! A full queue drops messages instead of asserting
    void testQueueOverflowDrops();

  private:
    // ----------------------------------------------------------------------
    // Handlers for typed from ports
    // ----------------------------------------------------------------------

    //! Copies each frame (the component's buffer is only valid during the call)
    //! and fails the send selected by m_failSendIndex
    Drv::ByteStreamStatus from_toByteStreamDriver_handler(FwIndexType portNum, Fw::Buffer& sendBuffer) override;

  private:
    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

    //! Set the component's parameters and load them
    void setParams(I32 minPosition,
                   I32 maxPosition,
                   U32 advanceVelocity = 0,
                   U32 retractVelocity = 0,
                   U32 acceleration = 0,
                   U8 nodeId = 1);

    //! Invoke an async port and dispatch it
    void enable();
    void moveTo(I32 position);
    void stop();

    //! Decode a recorded frame, checking the Waveshare framing
    Sdo decode(FwSizeType frameIndex) const;

    //! Check that frame frameIndex is an SDO download of value to index with the given size
    void assertSdo(FwSizeType frameIndex, U16 index, U32 value, U8 size, U8 nodeId = 1) const;

    //! Forget recorded frames and histories
    void clearAll();

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    PlatformMotor component;

    //! Every frame sent to the driver, in order
    std::vector<Frame> m_frames;

    //! Number of sends attempted (including failed ones)
    FwSizeType m_sendAttempts = 0;

    //! Index of the send attempt to fail (counting from 0), or -1 for none
    I32 m_failSendIndex = -1;
};

}  // namespace Mara

#endif
