// ======================================================================
// \title  PlatformMotor.cpp
// \author fsowa
// \brief  cpp file for PlatformMotor component implementation class
// ======================================================================

#include "Mara/Components/PlatformMotor/PlatformMotor.hpp"

#include "Os/Task.hpp"

#include <algorithm>
#include <array>

namespace Mara {

namespace {
// CiA 402 objects in the drive
constexpr U16 CONTROLWORD = 0x6040;
constexpr U16 MODE_OF_OPERATION = 0x6060;
constexpr U16 TARGET_POSITION = 0x607A;
constexpr U16 PROFILE_VELOCITY = 0x6081;
constexpr U16 PROFILE_ACCELERATION = 0x6083;

constexpr U32 PROFILE_POSITION_MODE = 1;

// Controlword values
constexpr U32 CW_FAULT_RESET = 0x0080;
constexpr U32 CW_SHUTDOWN = 0x0006;
constexpr U32 CW_SWITCH_ON = 0x0007;
constexpr U32 CW_ENABLE = 0x000F;      // operation enabled; also clears halt and new-setpoint
constexpr U32 CW_START_MOVE = 0x003F;  // enabled + new setpoint + change immediately, absolute
constexpr U32 CW_HALT = 0x010F;        // stop and hold
}  // namespace

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

PlatformMotor ::PlatformMotor(const char* const compName) : PlatformMotorComponentBase(compName) {}

PlatformMotor ::~PlatformMotor() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void PlatformMotor ::byteStreamDriverReady_handler(FwIndexType portNum) {
    (void)portNum;
}

void PlatformMotor ::enable_handler(FwIndexType portNum) {
    (void)portNum;
    sdoWrite(MODE_OF_OPERATION, PROFILE_POSITION_MODE, 1);

    Fw::ParamValid valid;
    const U32 acceleration = this->paramGet_ACCELERATION(valid);
    if (acceleration != 0) {
        sdoWrite(PROFILE_ACCELERATION, acceleration, 4);
    }

    sdoWrite(CONTROLWORD, CW_FAULT_RESET, 2);
    sdoWrite(CONTROLWORD, CW_SHUTDOWN, 2);
    sdoWrite(CONTROLWORD, CW_SWITCH_ON, 2);
    sdoWrite(CONTROLWORD, CW_ENABLE, 2);
    this->log_ACTIVITY_HI_Enabled();
}

void PlatformMotor ::fromByteStreamDriver_handler(FwIndexType portNum,
                                                  Fw::Buffer& buffer,
                                                  const Drv::ByteStreamStatus& status) {
    (void)portNum;
    (void)status;
    // Drive replies are not parsed yet; hand the buffer back to the driver.
    this->fromByteStreamDriverReturn_out(0, buffer);
}

void PlatformMotor ::moveTo_handler(FwIndexType portNum, I32 position) {
    (void)portNum;
    Fw::ParamValid valid;
    const I32 minPosition = this->paramGet_MIN_POSITION(valid);
    const I32 maxPosition = this->paramGet_MAX_POSITION(valid);

    const I32 target = std::max(minPosition, std::min(position, maxPosition));
    if (target != position) {
        this->log_WARNING_LO_PositionClamped(position, target);
    }

    // Upward is the drilling feed, downward is the retract
    const U32 velocity =
        (target > m_target) ? this->paramGet_ADVANCE_VELOCITY(valid) : this->paramGet_RETRACT_VELOCITY(valid);
    if (velocity != 0) {
        sdoWrite(PROFILE_VELOCITY, velocity, 4);
    }

    sdoWrite(CONTROLWORD, CW_ENABLE, 2);  // clear new-setpoint so the next write is a rising edge
    sdoWrite(TARGET_POSITION, static_cast<U32>(target), 4);
    sdoWrite(CONTROLWORD, CW_START_MOVE, 2);

    m_target = target;
    this->tlmWrite_TargetPosition(target);
    this->log_ACTIVITY_HI_MovingTo(target);
}

void PlatformMotor ::pingIn_handler(FwIndexType portNum, U32 key) {
    (void)portNum;
    this->pingOut_out(0, key);
}

void PlatformMotor ::stop_handler(FwIndexType portNum) {
    (void)portNum;
    sdoWrite(CONTROLWORD, CW_HALT, 2);
    this->log_ACTIVITY_HI_Stopped();
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void PlatformMotor ::sdoWrite(U16 index, U32 value, U8 size) {
    Fw::ParamValid valid;
    const U16 cobId = 0x600 + this->paramGet_NODE_ID(valid);  // SDO request to this node

    // SDO expedited download: the command byte encodes the value size
    const U8 command = (size == 4) ? 0x23 : (size == 2) ? 0x2B : 0x2F;

    // Waveshare USB-CAN-A frame: header, type (standard data frame, 8 bytes),
    // CAN ID little-endian, 8 data bytes, end code. CANopen data is little-endian too.
    std::array<U8, 13> frame{0xAA,
                             0xC8,
                             static_cast<U8>(cobId),
                             static_cast<U8>(cobId >> 8),
                             command,
                             static_cast<U8>(index),
                             static_cast<U8>(index >> 8),
                             0x00,  // subindex
                             static_cast<U8>(value),
                             static_cast<U8>(value >> 8),
                             static_cast<U8>(value >> 16),
                             static_cast<U8>(value >> 24),
                             0x55};

    if (this->isConnected_toByteStreamDriver_OutputPort(0)) {
        Fw::Buffer buffer(frame.data(), frame.size());
        const Drv::ByteStreamStatus status = this->toByteStreamDriver_out(0, buffer);
        if (status != Drv::ByteStreamStatus::OP_OK) {
            this->log_WARNING_HI_SendFailed(status);
        }
    }

    // Give the drive time to handle this request before the next one
    (void)Os::Task::delay(Fw::TimeInterval(0, 10 * 1000));
}

}  // namespace Mara
