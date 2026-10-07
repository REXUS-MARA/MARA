// ======================================================================
// \title  PlatformMotor.cpp
// \author fsowa
// \brief  cpp file for PlatformMotor component implementation class
// ======================================================================

#include "Mara/Components/PlatformMotor/PlatformMotor.hpp"

#include "Fw/Types/Serializable.hpp"
#include "Os/Task.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace Mara {

namespace {
// CiA 402 objects in the drive
constexpr U16 CONTROLWORD = 0x6040;
constexpr U16 MODE_OF_OPERATION = 0x6060;
constexpr U16 TARGET_POSITION = 0x607A;
constexpr U16 PROFILE_VELOCITY = 0x6081;
constexpr U16 PROFILE_ACCELERATION = 0x6083;

constexpr U32 PROFILE_POSITION_MODE = 1;

// Waveshare USB-CAN-A variable-length frame: header, type, CAN ID (LE), data, end code
constexpr U8 FRAME_HEADER = 0xAA;
constexpr U8 FRAME_TYPE_STD_DATA_8 = 0xC8;  // standard 11-bit ID, data frame, DLC 8
constexpr U8 FRAME_END = 0x55;
constexpr FwSizeType FRAME_SIZE = 13;  // header + type + ID(2) + 8 data bytes + end

// CANopen SDO expedited download command bytes, by value size
constexpr U8 SDO_DOWNLOAD_1 = 0x2F;
constexpr U8 SDO_DOWNLOAD_2 = 0x2B;
constexpr U8 SDO_DOWNLOAD_4 = 0x23;

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
    Fw::ParamValid valid;
    const I32 minPosition = this->paramGet_MIN_POSITION(valid);
    const I32 maxPosition = this->paramGet_MAX_POSITION(valid);
    if (minPosition == 0 && maxPosition == 0) {
        this->log_WARNING_HI_LimitsUnset();
    } else if (minPosition > 0 || maxPosition < 0) {
        // Also covers MIN > MAX. The retract to 0 could not reach the bottom.
        this->log_WARNING_HI_LimitsInvalid(minPosition, maxPosition);
    }

    if (!sdoWriteAll({{MODE_OF_OPERATION, PROFILE_POSITION_MODE, 1}})) {
        return;
    }

    const U32 acceleration = this->paramGet_ACCELERATION(valid);
    if (acceleration != 0 && !sdoWriteAll({{PROFILE_ACCELERATION, acceleration, 4}})) {
        return;
    }

    if (!sdoWriteAll({{CONTROLWORD, CW_FAULT_RESET, 2},
                      {CONTROLWORD, CW_SHUTDOWN, 2},
                      {CONTROLWORD, CW_SWITCH_ON, 2},
                      {CONTROLWORD, CW_ENABLE, 2}})) {
        return;
    }
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
    if (minPosition > maxPosition) {
        // The clamp below would send every target, even the retract to 0, to MIN_POSITION
        this->log_WARNING_HI_LimitsInvalid(minPosition, maxPosition);
        return;
    }

    const I32 target = std::max(minPosition, std::min(position, maxPosition));
    if (target != position) {
        this->log_WARNING_LO_PositionClamped(position, target);
    }

    // Away from 0 is the drilling feed, towards 0 is the retract. Comparing distances from 0
    // keeps this right whichever sign the drive uses for "towards the sample".
    const U32 velocity = (std::abs(target) > std::abs(m_target)) ? this->paramGet_ADVANCE_VELOCITY(valid)
                                                                 : this->paramGet_RETRACT_VELOCITY(valid);
    if (velocity != 0 && !sdoWriteAll({{PROFILE_VELOCITY, velocity, 4}})) {
        return;
    }

    if (!sdoWriteAll({{CONTROLWORD, CW_ENABLE, 2},  // clear new-setpoint so the next write is a rising edge
                      {TARGET_POSITION, static_cast<U32>(target), 4},
                      {CONTROLWORD, CW_START_MOVE, 2}})) {
        return;
    }

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
    if (!sdoWriteAll({{CONTROLWORD, CW_HALT, 2}})) {
        return;
    }
    this->log_ACTIVITY_HI_Stopped();
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

bool PlatformMotor ::sdoWriteAll(std::initializer_list<SdoWrite> writes) {
    Fw::ParamValid valid;
    const U8 nodeId = this->paramGet_NODE_ID(valid);
    if (nodeId < 1 || nodeId > 127) {
        // Outside CANopen's node range the COB-ID would not address the drive's SDO server
        this->log_WARNING_HI_NodeIdInvalid(nodeId);
        return false;
    }
    const U16 cobId = 0x600 + nodeId;  // SDO request to this node

    for (const SdoWrite& write : writes) {
        const U8 command = (write.size == 4) ? SDO_DOWNLOAD_4 : (write.size == 2) ? SDO_DOWNLOAD_2 : SDO_DOWNLOAD_1;

        // CAN ID and CANopen data are little-endian. The 8 data bytes are the SDO command,
        // index, subindex and a 4-byte value (unused high bytes are 0 for 1/2-byte objects).
        std::array<U8, FRAME_SIZE> frame{};
        Fw::ExternalSerializeBuffer serializer(frame.data(), frame.size());
        const Fw::Endianness le = Fw::Endianness::LITTLE;
        const bool built = serializer.serializeFrom(FRAME_HEADER) == Fw::FW_SERIALIZE_OK &&
                           serializer.serializeFrom(FRAME_TYPE_STD_DATA_8) == Fw::FW_SERIALIZE_OK &&
                           serializer.serializeFrom(cobId, le) == Fw::FW_SERIALIZE_OK &&
                           serializer.serializeFrom(command) == Fw::FW_SERIALIZE_OK &&
                           serializer.serializeFrom(write.index, le) == Fw::FW_SERIALIZE_OK &&
                           serializer.serializeFrom(static_cast<U8>(0)) == Fw::FW_SERIALIZE_OK &&  // subindex
                           serializer.serializeFrom(write.value, le) == Fw::FW_SERIALIZE_OK &&
                           serializer.serializeFrom(FRAME_END) == Fw::FW_SERIALIZE_OK &&
                           serializer.getSize() == FRAME_SIZE;

        Drv::ByteStreamStatus status = Drv::ByteStreamStatus::OTHER_ERROR;
        if (built && this->isConnected_toByteStreamDriver_OutputPort(0)) {
            Fw::Buffer buffer(frame.data(), frame.size());
            status = this->toByteStreamDriver_out(0, buffer);
        }
        if (status != Drv::ByteStreamStatus::OP_OK) {
            this->log_WARNING_HI_SendFailed(status);
            return false;
        }

        // Give the drive time to handle this request before the next one
        (void)Os::Task::delay(Fw::TimeInterval(0, 10 * 1000));
    }
    return true;
}

}  // namespace Mara
