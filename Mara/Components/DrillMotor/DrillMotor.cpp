// ======================================================================
// \title  DrillMotor.cpp
// \author john
// \brief  cpp file for DrillMotor component implementation class
// ======================================================================

#include "Mara/Components/DrillMotor/DrillMotor.hpp"
#include <Fw/Types/String.hpp>

namespace Mara {


F64 secondsBetween(const Fw::Time& start, const Fw::Time& end) {
    return (static_cast<F64>(end.getSeconds()) - static_cast<F64>(start.getSeconds())) +
           (static_cast<F64>(end.getUSeconds()) - static_cast<F64>(start.getUSeconds())) / 1.0e6;
} //namespace
 
// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

DrillMotor ::DrillMotor(const char* const compName) : DrillMotorComponentBase(compName) {}
 
DrillMotor ::~DrillMotor() {}

void DrillMotor ::updateRpm() {
    const Fw::Time now = this->getTime();
 
    // First call: just open the first measurement window
    if (not this->m_windowStarted) {
        this->m_windowStart = now;
        this->m_pulseCount.store(0);
        this->m_windowStarted = true;
        return;
    }
 
    const F64 elapsed = secondsBetween(this->m_windowStart, now);
 
    // Clock stepped backwards: restart the window rather than stalling
    if (elapsed < 0.0) {
        this->m_windowStart = now;
        this->m_pulseCount.store(0);
        return;
    }
 
    // Window not finished yet
    if (elapsed < MEASUREMENT_WINDOW_SEC) {
        return;
    }
 
    // Window finished: take the count and start the next window
    const U32 pulses = this->m_pulseCount.exchange(0);
    this->m_windowStart = now;
 
    // Long stall or clock jump: this window's rate would be meaningless
    if (elapsed > MAX_VALID_WINDOW_SEC) {
        return;
    }
 
    // rpm = (pulses / seconds) * 60 / pulses-per-rev
    const F32 rpm = static_cast<F32>((static_cast<F64>(pulses) / elapsed) * 60.0 /
                                     static_cast<F64>(PULSES_PER_REV));
 
    this->m_rpm.store(rpm);
    this->tlmWrite_MotorRpm(rpm);
    this->recordRpmSample(rpm);
}
 
void DrillMotor ::recordRpmSample(F32 rpm) {
    // Get a new container if we don't currently hold one
    if (not this->m_containerValid) {
        const FwSizeType containerSize = RECORD_COUNT * (MotorRpmSample::SERIALIZED_SIZE + sizeof(FwDpIdType));
 
        Fw::Success dp_status = this->dpGet_MotorRpmContainer(containerSize, this->m_container);
        if (dp_status != Fw::Success::SUCCESS) {
            this->log_WARNING_HI_DpMemoryFailure(containerSize);
            return;
        }
 
        this->m_containerValid = true;
        this->m_container.setTimeTag(this->getTime());
        this->log_WARNING_HI_DpMemoryFailure_ThrottleClear();
    }
 
    // Build the timestamped sample
    Fw::Time currentFwTime = this->getTime();
    Fw::TimeValue currentTime = Fw::TimeValue(currentFwTime.getTimeBase(), currentFwTime.getContext(),
                                              currentFwTime.getSeconds(), currentFwTime.getUSeconds());
    MotorRpmSample sample;
    sample.set_time_stamp(currentTime);
    sample.set_rpm(rpm);
 
    // Serialize the sample into the container
    Fw::SerializeStatus serialize_status = this->m_container.serializeRecord_MotorRpmRecord(sample);
    FW_ASSERT(serialize_status == Fw::SerializeStatus::FW_SERIALIZE_OK);
    this->m_count += 1;
 
    // Once the container is full, send it and start fresh next time
    if (this->m_count == RECORD_COUNT) {
        this->dpSend(this->m_container);
        this->m_count = 0;
        this->m_containerValid = false;
    }
}
 

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------
 
void DrillMotor ::MOTOR_ON_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    FW_ASSERT(this->isConnected_motorPinWrite_OutputPort(0));
 
    Drv::GpioStatus status = this->motorPinWrite_out(0, Fw::Logic::HIGH);
 
    if (status != Drv::GpioStatus::OP_OK) {
        this->log_WARNING_HI_GpioError(Fw::String("MOTOR_ON"), static_cast<I32>(status.e));
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
 
    m_motorOn = true;
    this->tlmWrite_MotorState(true);
    this->log_ACTIVITY_HI_MotorOn();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void DrillMotor ::MOTOR_OFF_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    FW_ASSERT(this->isConnected_motorPinWrite_OutputPort(0));
 
    Drv::GpioStatus status = this->motorPinWrite_out(0, Fw::Logic::LOW);
 
    if (status != Drv::GpioStatus::OP_OK) {
        this->log_WARNING_HI_GpioError(Fw::String("MOTOR_OFF"), static_cast<I32>(status.e));
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
 
    m_motorOn = false;
    this->tlmWrite_MotorState(false);
    this->log_ACTIVITY_HI_MotorOff();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void DrillMotor ::SET_DIRECTION_cmdHandler(FwOpcodeType opCode,
                                            U32 cmdSeq,
                                            DrillMotor_Direction direction) {
    FW_ASSERT(this->isConnected_directionPinWrite_OutputPort(0));
 
    const Fw::Logic pinValue =
        (direction == DrillMotor_Direction::CW) ? Fw::Logic::HIGH : Fw::Logic::LOW;
 
    Drv::GpioStatus status = this->directionPinWrite_out(0, pinValue);
 
    if (status != Drv::GpioStatus::OP_OK) {
        this->log_WARNING_HI_GpioError(Fw::String("SET_DIRECTION"), static_cast<I32>(status.e));
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
 
    m_direction = direction;
    this->tlmWrite_MotorDirection(direction);
    this->log_ACTIVITY_HI_DirectionChanged(direction);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void DrillMotor ::READ_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->log_ACTIVITY_LO_MotorRpmReported(this->m_rpm.load());
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void DrillMotor ::tachPulseIn_handler(FwIndexType portNum, Os::RawTime& cycleStart) {
    // Runs on the GPIO driver's interrupt thread: just count the edge
    this->m_pulseCount.fetch_add(1, std::memory_order_relaxed);
}


void DrillMotor ::schedIn_handler(FwIndexType portNum, U32 context) {
    this->updateRpm();
}
 


}  // namespace Mara
