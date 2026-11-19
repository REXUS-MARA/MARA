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
#include <cmath>
#include <cstdlib>

namespace Mara {

namespace {
// CiA 402 objects in the drive
constexpr U16 CONTROLWORD = 0x6040;
constexpr U16 STATUSWORD = 0x6041;
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

constexpr U8 FRAME_TYPE_MASK = 0xC0;
constexpr U8 FRAME_TYPE_EXTENDED = 0x20;
constexpr U8 FRAME_TYPE_REMOTE = 0x10;
constexpr U8 FRAME_TYPE_DLC = 0x0F;

constexpr FwSizeType SETTINGS_SIZE = 20;
constexpr U8 SETTINGS_BITRATE_500K = 0x03;  // must match the drive's CAN bit rate
constexpr U8 SETTINGS_FRAME_STANDARD = 0x01;
constexpr U8 SETTINGS_MODE_NORMAL = 0x00;


// CANopen SDO expedited download command bytes, by value size
constexpr U8 SDO_DOWNLOAD_1 = 0x2F;
constexpr U8 SDO_DOWNLOAD_2 = 0x2B;
constexpr U8 SDO_DOWNLOAD_4 = 0x23;
constexpr U8 SDO_UPLOAD_REQUEST = 0x40;
constexpr U8 SDO_UPLOAD_REPLY_2 = 0x4B;  // expedited upload reply with a 2-byte value
constexpr U8 SDO_ABORT = 0x80;

constexpr U32 COB_EMERGENCY = 0x080;
constexpr U32 COB_SDO_REPLY = 0x580;

constexpr U16 SW_FAULT = 0x0008;
constexpr U16 SW_TARGET_REACHED = 0x0400;
constexpr U16 SW_SETPOINT_ACK = 0x1000; 


// Controlword values
constexpr U32 CW_FAULT_RESET = 0x0080;
constexpr U32 CW_SHUTDOWN = 0x0006;
constexpr U32 CW_SWITCH_ON = 0x0007;
constexpr U32 CW_ENABLE = 0x000F;      // operation enabled; also clears halt and new-setpoint
constexpr U32 CW_START_MOVE = 0x003F;  // enabled + new setpoint + change immediately, absolute
constexpr U32 CW_HALT = 0x010F;        // stop and hold

constexpr F64 FIXED_POINT_SCALE = 65536.0;
constexpr F64 SAMPLE_PERIOD_S = 0.001;  // drive slow-loop period
constexpr F64 MOVE_MARGIN_S = 1.0;      // extra time for the move to settle before the next step
constexpr F64 MOVE_TIMEOUT_EXTRA_S = 10.0;  // a move is reported as timed out after 2x its estimate plus this
constexpr F64 MOVE_TIMEOUT_UNKNOWN_S = 600.0;  // timeout when speed or acceleration is left to the drive
 
constexpr U32 SDO_GAP_MS = 20;          // pause after each SDO so the drive has replied before the next
constexpr U32 ENABLE_SETTLE_MS = 500;   // let the closed loop settle after enabling before any move

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
    (void)this->configureAdapter();
}

void PlatformMotor ::enable_handler(FwIndexType portNum) {
    (void)portNum;
    (void)this->enableDrive();
}

void PlatformMotor ::fromByteStreamDriver_handler(FwIndexType portNum,
                                                  Fw::Buffer& buffer,
                                                  const Drv::ByteStreamStatus& status) {
    (void)portNum;
    if (status == Drv::ByteStreamStatus::OP_OK && buffer.isValid()) {
        const U8* bytes = buffer.getData();
        const FwSizeType count = buffer.getSize();
        for (FwSizeType i = 0; i < count; i++) {
            if (m_rxLength >= m_rx.size()) {
                m_rxLength = 0;  // never a valid frame this long; drop it and resync
            }
            m_rx[m_rxLength++] = bytes[i];
 
            // Resync on the header byte, then wait for a whole frame: AA, type, ID, data, 55
            if (m_rx[0] != FRAME_HEADER) {
                m_rxLength = 0;
                continue;
            }
            if (m_rxLength < 2) {
                continue;
            }
            const U8 type = m_rx[1];
            if ((type & FRAME_TYPE_MASK) != FRAME_TYPE_MASK) {
                m_rxLength = 0;  // not a CAN frame (e.g. a settings reply); skip it
                continue;
            }
            const FwSizeType idLength = (type & FRAME_TYPE_EXTENDED) ? 4 : 2;
            const U8 dlc = static_cast<U8>(type & FRAME_TYPE_DLC);
            const FwSizeType frameLength = 2 + idLength + dlc + 1;
            if (dlc > 8) {
                m_rxLength = 0;
                continue;
            }
            if (m_rxLength < frameLength) {
                continue;
            }
            if (m_rx[frameLength - 1] == FRAME_END && !(type & FRAME_TYPE_REMOTE)) {
                U32 canId = 0;
                for (FwSizeType b = 0; b < idLength; b++) {
                    canId |= static_cast<U32>(m_rx[2 + b]) << (8 * b);
                }
                this->handleCanFrame(canId, &m_rx[2 + idLength], dlc);
            }
            m_rxLength = 0;
        }
    }
    this->fromByteStreamDriverReturn_out(0, buffer);
}


void PlatformMotor ::moveTo_handler(FwIndexType portNum, I32 position) {
    (void)portNum;
    // A move from the Orchestrator takes over from a running test cycle
    if (m_cycleStep != CycleStep::IDLE) {
        this->abortCycle();
    }
    (void)this->startMove(position);
}


void PlatformMotor ::pingIn_handler(FwIndexType portNum, U32 key) {
    (void)portNum;
    this->pingOut_out(0, key);
}

void PlatformMotor ::runTestCycle_handler(FwIndexType portNum) {
    (void)portNum;
    Fw::ParamValid valid;
    m_cycleStep = CycleStep::IDLE;
    if (!this->enableDrive()) {
        return;
    }
    this->log_ACTIVITY_HI_CycleStarted();
    this->cycleMove(this->paramGet_CYCLE_DISTANCE(valid), CycleStep::ADVANCE_1, true);
}

void PlatformMotor ::schedIn_handler(FwIndexType portNum, U32 context) {
    (void)portNum;
    (void)context;
    const F64 now = this->nowSeconds();
 
    if (m_moveActive) {
        if (now > m_moveDeadline) {
            m_moveActive = false;
            this->log_WARNING_HI_MoveTimedOut(m_target);
            if (m_cycleStep != CycleStep::IDLE) {
                this->abortCycle();
            }
        } else if (this->sendSdo(SDO_UPLOAD_REQUEST, STATUSWORD, 0)) {
            // The reply arrives through statusWordReceived
            m_pollsSinceStart++;
        }
    }
 
    // A cycle step whose move is done and whose pause is over
    if (m_cycleStep != CycleStep::IDLE && !m_cycleWaitingForMove && now >= m_stepDeadline) {
        this->advanceCycle();
    }
}



void PlatformMotor ::stop_handler(FwIndexType portNum) {
    (void)portNum;
    if (m_cycleStep != CycleStep::IDLE) {
        this->abortCycle();
    }
    // A halted move never reaches its target, so stop watching for it
    m_moveActive = false;
    if (!sdoWriteAll({{CONTROLWORD, CW_HALT, 2}})) {
        return;
    }
    this->log_ACTIVITY_HI_Stopped();
}


// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void PlatformMotor ::statusWordReceived_internalInterfaceHandler(U16 statusWord) {
    this->tlmWrite_StatusWord(statusWord);
    if (!m_moveActive || m_pollsSinceStart == 0) {
        return;  // not watching a move, or a reply to a poll from before this move started
    }
 
    if (statusWord & SW_FAULT) {
        m_moveActive = false;
        this->log_WARNING_HI_DriveFault(statusWord);
        if (m_cycleStep != CycleStep::IDLE) {
            this->abortCycle();
        }
        return;
    }
 
    // Set-point acknowledge shows the drive took this move's target; target reached then means arrived
    if ((statusWord & SW_SETPOINT_ACK) && (statusWord & SW_TARGET_REACHED)) {
        this->onTargetReached();
    }
}


void PlatformMotor ::emergencyReceived_internalInterfaceHandler(U16 errorCode, U8 errorRegister) {
    // Log only. Communication errors (e.g. 0x7500, 0x1000) don't stop the drive; real faults
    // show up in the status word, which the move tracking already checks (DriveFault).
    this->log_WARNING_HI_DriveEmergency(errorCode, errorRegister);
}

void PlatformMotor ::sdoAbortReceived_internalInterfaceHandler(U16 index, U32 abortCode) {
    this->log_WARNING_HI_SdoAborted(index, abortCode);
}


bool PlatformMotor ::configureAdapter() {
    std::array<U8, SETTINGS_SIZE> settings{};
    settings[0] = 0xAA;
    settings[1] = 0x55;
    settings[2] = 0x12;
    settings[3] = SETTINGS_BITRATE_500K;
    settings[4] = SETTINGS_FRAME_STANDARD;
    // settings[5..12]: filter ID and mask all 0, so every frame is received
    settings[13] = SETTINGS_MODE_NORMAL;
    settings[14] = 0x01;
    // settings[15..18]: reserved, 0
    U32 sum = 0;
    for (FwSizeType i = 2; i < SETTINGS_SIZE - 1; i++) {
        sum += settings[i];
    }
    settings[SETTINGS_SIZE - 1] = static_cast<U8>(sum & 0xFF);
 
    Drv::ByteStreamStatus status = Drv::ByteStreamStatus::OTHER_ERROR;
    if (this->isConnected_toByteStreamDriver_OutputPort(0)) {
        Fw::Buffer buffer(settings.data(), settings.size());
        status = this->toByteStreamDriver_out(0, buffer);
    }
    if (status != Drv::ByteStreamStatus::OP_OK) {
        this->log_WARNING_HI_SendFailed(status);
        return false;
    }
 
    // Give the adapter time to apply the settings before the first CAN frame
    (void)Os::Task::delay(Fw::TimeInterval(0, 100 * 1000));
    return true;
}

bool PlatformMotor ::enableDrive() {
    if (!this->configureAdapter()) {
        return false;
    }
 
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
        return false;
    }
 
    const U32 acceleration = this->paramGet_ACCELERATION(valid);
    if (acceleration != 0 && !sdoWriteAll({{PROFILE_ACCELERATION, acceleration, 4}})) {
        return false;
    }
 
    if (!sdoWriteAll({{CONTROLWORD, CW_FAULT_RESET, 2},
                      {CONTROLWORD, CW_SHUTDOWN, 2},
                      {CONTROLWORD, CW_SWITCH_ON, 2},
                      {CONTROLWORD, CW_ENABLE, 2}})) {
        return false;
    }
    // A move started straight after enabling can trip the following error before the loop has settled
    (void)Os::Task::delay(Fw::TimeInterval(0, ENABLE_SETTLE_MS * 1000));
    this->log_ACTIVITY_HI_Enabled();
    return true;
}

bool PlatformMotor ::startMove(I32 position) {
    Fw::ParamValid valid;
    const I32 minPosition = this->paramGet_MIN_POSITION(valid);
    const I32 maxPosition = this->paramGet_MAX_POSITION(valid);
    if (minPosition > maxPosition) {
        // The clamp below would send every target, even the retract to 0, to MIN_POSITION
        this->log_WARNING_HI_LimitsInvalid(minPosition, maxPosition);
        return false;
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
        return false;
    }
 
    if (!sdoWriteAll({{CONTROLWORD, CW_ENABLE, 2},  // clear new-setpoint so the next write is a rising edge
                      {TARGET_POSITION, static_cast<U32>(target), 4},
                      {CONTROLWORD, CW_START_MOVE, 2}})) {
        return false;
    }
 
    // Watch for the target to be reached, with a generous timeout from the expected move time
    const U32 distance = static_cast<U32>(std::abs(static_cast<I64>(target) - static_cast<I64>(m_target)));
    const U32 acceleration = this->paramGet_ACCELERATION(valid);
    const F64 expected = (velocity != 0 && acceleration != 0)
                             ? this->estimateMoveSeconds(distance, velocity, acceleration)
                             : MOVE_TIMEOUT_UNKNOWN_S / 2.0;
    m_moveActive = true;
    m_pollsSinceStart = 0;
    m_moveDeadline = this->nowSeconds() + 2.0 * expected + MOVE_TIMEOUT_EXTRA_S;
 
    m_target = target;
    this->tlmWrite_TargetPosition(target);
    this->log_ACTIVITY_HI_MovingTo(target);
    return true;
}

void PlatformMotor ::cycleMove(I32 position, CycleStep step, bool pauseAfter) {
    if (!this->startMove(position)) {
        this->abortCycle();
        return;
    }
    m_cycleStep = step;
    m_cyclePauseAfter = pauseAfter;
    m_cycleWaitingForMove = true;
}

void PlatformMotor ::onTargetReached() {
    m_moveActive = false;
    this->log_ACTIVITY_HI_TargetReached(m_target);
    if (this->isConnected_moveDone_OutputPort(0)) {
        this->moveDone_out(0);
    }
 
    if (m_cycleStep == CycleStep::IDLE || !m_cycleWaitingForMove) {
        return;
    }
    m_cycleWaitingForMove = false;
    if (m_cyclePauseAfter) {
        // schedIn moves on once the pause is over
        Fw::ParamValid valid;
        m_stepDeadline = this->nowSeconds() + static_cast<F64>(this->paramGet_CYCLE_PAUSE_MS(valid)) / 1000.0;
    } else {
        this->advanceCycle();
    }
}

void PlatformMotor ::advanceCycle() {
    Fw::ParamValid valid;
    switch (m_cycleStep) {
        case CycleStep::ADVANCE_1:
            // First advance and pause done: retract to the bottom, then pause again
            this->cycleMove(0, CycleStep::RETRACT, true);
            break;
        case CycleStep::RETRACT:
            // Retract and pause done: advance again and stay there
            this->cycleMove(this->paramGet_CYCLE_DISTANCE(valid), CycleStep::ADVANCE_2, false);
            break;
        case CycleStep::ADVANCE_2:
            // The drive stays enabled and holds the position until it is shut off
            m_cycleStep = CycleStep::IDLE;
            this->log_ACTIVITY_HI_CycleDone();
            break;
        default:
            m_cycleStep = CycleStep::IDLE;
            break;
    }
}
 
void PlatformMotor ::abortCycle() {
    m_cycleStep = CycleStep::IDLE;
    m_cycleWaitingForMove = false;
    this->log_WARNING_HI_CycleAborted();
}

F64 PlatformMotor ::estimateMoveSeconds(U32 distance, U32 velocity, U32 acceleration) const {
    if (distance == 0) {
        return 0.0;
    }
    const F64 v = static_cast<F64>(velocity) / FIXED_POINT_SCALE / SAMPLE_PERIOD_S;  // counts/s
    const F64 a =
        static_cast<F64>(acceleration) / FIXED_POINT_SCALE / (SAMPLE_PERIOD_S * SAMPLE_PERIOD_S);  // counts/s^2
    const F64 d = static_cast<F64>(distance);
 
    // Trapezoidal profile; a short move that never reaches full speed is triangular
    const F64 seconds = (d >= v * v / a) ? d / v + v / a : 2.0 * std::sqrt(d / a);
    return seconds + MOVE_MARGIN_S;
}

F64 PlatformMotor ::nowSeconds() {
    const Fw::Time now = this->getTime();
    return static_cast<F64>(now.getSeconds()) + static_cast<F64>(now.getUSeconds()) / 1.0e6;
}


bool PlatformMotor ::sdoWriteAll(std::initializer_list<SdoWrite> writes) {
    for (const SdoWrite& write : writes) {
        const U8 command = (write.size == 4) ? SDO_DOWNLOAD_4 : (write.size == 2) ? SDO_DOWNLOAD_2 : SDO_DOWNLOAD_1;
        if (!this->sendSdo(command, write.index, write.value)) {
            return false;
        }
        // Give the drive time to handle this request before the next one
        (void)Os::Task::delay(Fw::TimeInterval(0, SDO_GAP_MS * 1000));
    }
    return true;
}

bool PlatformMotor ::sendSdo(U8 command, U16 index, U32 value) {
    Fw::ParamValid valid;
    const U8 nodeId = this->paramGet_NODE_ID(valid);
    if (nodeId < 1 || nodeId > 127) {
        // Outside CANopen's node range the COB-ID would not address the drive's SDO server
        this->log_WARNING_HI_NodeIdInvalid(nodeId);
        return false;
    }
    const U16 cobId = 0x600 + nodeId;  // SDO request to this node
 
    // CAN ID and CANopen data are little-endian. The 8 data bytes are the SDO command,
    // index, subindex and a 4-byte value (unused high bytes are 0 for 1/2-byte objects and reads).
    std::array<U8, FRAME_SIZE> frame{};
    Fw::ExternalSerializeBuffer serializer(frame.data(), frame.size());
    const Fw::Endianness le = Fw::Endianness::LITTLE;
    const bool built = serializer.serializeFrom(FRAME_HEADER) == Fw::FW_SERIALIZE_OK &&
                       serializer.serializeFrom(FRAME_TYPE_STD_DATA_8) == Fw::FW_SERIALIZE_OK &&
                       serializer.serializeFrom(cobId, le) == Fw::FW_SERIALIZE_OK &&
                       serializer.serializeFrom(command) == Fw::FW_SERIALIZE_OK &&
                       serializer.serializeFrom(index, le) == Fw::FW_SERIALIZE_OK &&
                       serializer.serializeFrom(static_cast<U8>(0)) == Fw::FW_SERIALIZE_OK &&  // subindex
                       serializer.serializeFrom(value, le) == Fw::FW_SERIALIZE_OK &&
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
    return true;
}

void PlatformMotor ::handleCanFrame(U32 canId, const U8* data, U8 length) {
    Fw::ParamValid valid;
    const U8 nodeId = this->paramGet_NODE_ID(valid);
 
    if (canId == COB_SDO_REPLY + nodeId && length == 8) {
        const U16 index = static_cast<U16>(data[1] | (data[2] << 8));
        if (data[0] == SDO_UPLOAD_REPLY_2 && index == STATUSWORD) {
            this->statusWordReceived_internalInterfaceInvoke(static_cast<U16>(data[4] | (data[5] << 8)));
        } else if (data[0] == SDO_ABORT) {
            const U32 abortCode = static_cast<U32>(data[4]) | (static_cast<U32>(data[5]) << 8) |
                                  (static_cast<U32>(data[6]) << 16) | (static_cast<U32>(data[7]) << 24);
            this->sdoAbortReceived_internalInterfaceInvoke(index, abortCode);
        }
        // Write confirmations (0x60) need no action
    } else if (canId == COB_EMERGENCY + nodeId && length >= 3) {
        const U16 errorCode = static_cast<U16>(data[0] | (data[1] << 8));
        if (errorCode != 0) {  // 0 means "error reset", not a new error
            this->emergencyReceived_internalInterfaceInvoke(errorCode, data[2]);
        }
    }
}


}  // namespace Mara
