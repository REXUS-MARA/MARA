// ======================================================================
// \title  PlatformMotor.cpp
// \author fsowa
// \brief  cpp file for PlatformMotor component implementation class
// ======================================================================

#include "Mara/Components/PlatformMotor/PlatformMotor.hpp"

#include "Fw/Types/Serializable.hpp"
#include "Os/File.hpp"
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
constexpr U16 POSITION_ACTUAL = 0x6064;

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
constexpr U8 SDO_UPLOAD_REPLY_4 = 0x43;
constexpr U8 SDO_ABORT = 0x80;

constexpr U32 COB_EMERGENCY = 0x080;
constexpr U32 COB_SDO_REPLY = 0x580;
constexpr U32 COB_BOOTUP = 0x700;


constexpr U16 SW_FAULT = 0x0008;
constexpr U16 SW_TARGET_REACHED = 0x0400;
constexpr U16 SW_SETPOINT_ACK = 0x1000; 
constexpr U16 SW_STATE_MASK = 0x004F;
constexpr U16 SW_STATE_SWITCH_ON_DISABLED = 0x0040;


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

constexpr U32 POS_MAGIC = 0x534F504D;  // "MPOS"
constexpr FwSizeType POS_RECORD_SIZE = 20;
constexpr U32 REST_SAVE_THRESHOLD = 20;  // at standstill, ignore encoder dither of a few counts
 
void putU32(U8* p, U32 v) {
    p[0] = static_cast<U8>(v);
    p[1] = static_cast<U8>(v >> 8);
    p[2] = static_cast<U8>(v >> 16);
    p[3] = static_cast<U8>(v >> 24);
}
 
U32 getU32(const U8* p) {
    return static_cast<U32>(p[0]) | (static_cast<U32>(p[1]) << 8) | (static_cast<U32>(p[2]) << 16) |
           (static_cast<U32>(p[3]) << 24);
}
 
//! FNV-1a over the record body, so a torn or corrupted write is detected
U32 checksum(const U8* p, FwSizeType n) {
    U32 h = 2166136261u;
    for (FwSizeType i = 0; i < n; i++) {
        h = (h ^ p[i]) * 16777619u;
    }
    return h;
}   

}  // namespace

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

PlatformMotor ::PlatformMotor(const char* const compName)
    : PlatformMotorComponentBase(compName), m_pathA("PlatformPositionA.bin"), m_pathB("PlatformPositionB.bin") {}

PlatformMotor ::~PlatformMotor() {}

void PlatformMotor ::configure(const char* pathA, const char* pathB) {
    m_pathA = pathA;
    m_pathB = pathB;
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void PlatformMotor ::byteStreamDriverReady_handler(FwIndexType portNum) {
    (void)portNum;
    m_uartReady = true;
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
        }
    }
 
    if (m_uartReady) {
        // The position is read on every tick: it keeps the stored copy current and, after a
        // restart, is what the stored position is restored against. Replies arrive through
        // positionReceived and statusWordReceived.
        (void)this->sendSdo(SDO_UPLOAD_REQUEST, POSITION_ACTUAL, 0);
        if (m_moveActive || m_restorePending) {
            (void)Os::Task::delay(Fw::TimeInterval(0, SDO_GAP_MS * 1000));
            if (this->sendSdo(SDO_UPLOAD_REQUEST, STATUSWORD, 0) && m_moveActive) {
                m_pollsSinceStart++;
            }
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
    m_lastStatus = statusWord;
    m_haveStatus = true;
    this->tryRestorePosition();
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
    //if ((statusWord & SW_SETPOINT_ACK) && (statusWord & SW_TARGET_REACHED)) {
    //    this->onTargetReached();
    //}
    const bool reached = (statusWord & SW_TARGET_REACHED) != 0;
    if (!reached) {
        m_sawMoving = true;  // the move is running
        return;
    }
    // Reached: accept it once the move was seen running, or if the drive acknowledged it,
    // or after a few polls for a move so short it finished between two polls
    if (m_sawMoving || (statusWord & SW_SETPOINT_ACK) || m_pollsSinceStart >= 3) {
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

void PlatformMotor ::positionReceived_internalInterfaceHandler(I32 drivePosition) {
    m_drivePos = drivePosition;
    m_haveDrivePos = true;
    if (m_restorePending) {
        this->tryRestorePosition();
        return;
    }
    if (!m_positionKnown) {
        return;
    }
 
    m_platformPos = static_cast<I32>(static_cast<I64>(drivePosition) + m_offset);
    this->tlmWrite_ActualPosition(m_platformPos);
 
    // Save often while moving, so a power cut loses at most the threshold. At standstill only
    // save a real change, not the closed loop's dither of a few counts.
    Fw::ParamValid valid;
    U32 threshold = m_moveActive ? this->paramGet_POSITION_SAVE_THRESHOLD(valid) : REST_SAVE_THRESHOLD;
    if (threshold == 0) {
        threshold = 1;
    }
    const I64 change = static_cast<I64>(m_platformPos) - static_cast<I64>(m_savedPos);
    if (static_cast<U64>(change < 0 ? -change : change) >= threshold) {
        this->savePosition();
    }
}

void PlatformMotor ::driveBooted_internalInterfaceHandler() {
    if (m_restorePending || !m_positionKnown) {
        return;  // the start-up restore handles a drive that boots with or after the software
    }
    // The drive lost power while the software kept running. Its counter restarts at 0, but the
    // platform did not move (the screw holds it), so keep the platform position.
    m_offset = m_platformPos;
    m_drivePos = 0;
    m_target = m_platformPos;
    if (m_moveActive) {
        m_moveActive = false;  // the move was cut off; nothing is restarted automatically
    }
    if (m_cycleStep != CycleStep::IDLE) {
        this->abortCycle();
    }
    this->log_WARNING_HI_DriveRestarted(m_platformPos);
    this->savePosition();
}

void PlatformMotor ::SET_POSITION_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, I32 position) {
    if (!m_haveDrivePos || m_moveActive) {
        this->log_WARNING_LO_SetPositionRefused();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    m_restorePending = false;
    m_offset = static_cast<I32>(static_cast<I64>(position) - m_drivePos);
    m_positionKnown = true;
    m_platformPos = position;
    m_target = position;
    this->tlmWrite_ActualPosition(position);
    this->tlmWrite_TargetPosition(position);
    this->savePosition();
    this->log_ACTIVITY_HI_PositionSet(position);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
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
    if (!m_positionKnown) {
    // Without a known position, MIN/MAX and "retract to 0" would mean nothing
    this->log_WARNING_HI_PositionUnknown();
    return false;
    }
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
                      {TARGET_POSITION, static_cast<U32>(static_cast<I32>(static_cast<I64>(target) - m_offset)), 4},
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
    m_sawMoving = false;
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


void PlatformMotor ::tryRestorePosition() {
    if (!m_restorePending || !m_haveStatus || !m_haveDrivePos) {
        return;
    }
    m_restorePending = false;
 
    I32 savedPosition = 0;
    I32 savedOffset = 0;
    if (!this->loadPosition(savedPosition, savedOffset)) {
        this->log_WARNING_HI_PositionUnknown();
        return;
    }
 
    // Right after power-up the drive is in "switch on disabled" and its counter restarted at 0,
    // so the platform is where it was last saved. Otherwise the drive kept power and kept counting,
    // so the saved offset still applies (and also covers movement after the last save).
    const bool driveRestarted = (m_lastStatus & SW_STATE_MASK) == SW_STATE_SWITCH_ON_DISABLED;
    m_offset = driveRestarted ? static_cast<I32>(static_cast<I64>(savedPosition) - m_drivePos) : savedOffset;
    m_positionKnown = true;
    m_platformPos = static_cast<I32>(static_cast<I64>(m_drivePos) + m_offset);
    m_target = m_platformPos;
    this->tlmWrite_ActualPosition(m_platformPos);
    this->tlmWrite_TargetPosition(m_target);
    this->log_ACTIVITY_HI_PositionRestored(m_platformPos, driveRestarted);
    this->savePosition();
}

void PlatformMotor ::savePosition() {
    const U32 seq = m_saveSeq + 1;
    U8 record[POS_RECORD_SIZE];
    putU32(&record[0], POS_MAGIC);
    putU32(&record[4], seq);
    putU32(&record[8], static_cast<U32>(m_platformPos));
    putU32(&record[12], static_cast<U32>(m_offset));
    putU32(&record[16], checksum(record, 16));
 
    // Alternate between the two files: a power cut during one write leaves the other intact
    const char* path = (seq & 1U) ? m_pathA.toChar() : m_pathB.toChar();
    Os::File file;
    bool ok = file.open(path, Os::File::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE) == Os::File::OP_OK;
    if (ok) {
        FwSizeType size = POS_RECORD_SIZE;
        ok = file.write(record, size, Os::File::WaitType::WAIT) == Os::File::OP_OK && size == POS_RECORD_SIZE &&
             file.flush() == Os::File::OP_OK;  // flush to the card, not just the OS cache
        file.close();
    }
    if (!ok) {
        this->log_WARNING_HI_PositionSaveFailed();
        return;
    }
    m_saveSeq = seq;
    m_savedPos = m_platformPos;
}

bool PlatformMotor ::loadPosition(I32& platformPosition, I32& offset) {
    bool found = false;
    U32 bestSeq = 0;
    const char* paths[2] = {m_pathA.toChar(), m_pathB.toChar()};
    for (const char* path : paths) {
        Os::File file;
        if (file.open(path, Os::File::OPEN_READ) != Os::File::OP_OK) {
            continue;
        }
        U8 record[POS_RECORD_SIZE];
        FwSizeType size = POS_RECORD_SIZE;
        const bool read = file.read(record, size, Os::File::WaitType::WAIT) == Os::File::OP_OK;
        file.close();
        if (!read || size != POS_RECORD_SIZE || getU32(&record[0]) != POS_MAGIC ||
            getU32(&record[16]) != checksum(record, 16)) {
            continue;  // missing, torn or corrupted
        }
        const U32 seq = getU32(&record[4]);
        if (!found || seq > bestSeq) {
            found = true;
            bestSeq = seq;
            platformPosition = static_cast<I32>(getU32(&record[8]));
            offset = static_cast<I32>(getU32(&record[12]));
        }
    }
    if (found) {
        m_saveSeq = bestSeq;
    }
    return found;
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
        } else if (data[0] == SDO_UPLOAD_REPLY_4 && index == POSITION_ACTUAL) {
            this->positionReceived_internalInterfaceInvoke(static_cast<I32>(getU32(&data[4]))); 
        } else if (data[0] == SDO_ABORT) {
            const U32 abortCode = static_cast<U32>(data[4]) | (static_cast<U32>(data[5]) << 8) |
                                  (static_cast<U32>(data[6]) << 16) | (static_cast<U32>(data[7]) << 24);
            this->sdoAbortReceived_internalInterfaceInvoke(index, abortCode);
        }
        // Write confirmations (0x60) need no action
    } else if (canId == COB_BOOTUP + nodeId && length == 1 && data[0] == 0x00) {
        // Boot-up message (heartbeats carry a non-zero state byte)
        this->driveBooted_internalInterfaceInvoke();
    } else if (canId == COB_EMERGENCY + nodeId && length >= 3) {
        const U16 errorCode = static_cast<U16>(data[0] | (data[1] << 8));
        if (errorCode != 0) {  // 0 means "error reset", not a new error
            this->emergencyReceived_internalInterfaceInvoke(errorCode, data[2]);
        }
    }
}


}  // namespace Mara
