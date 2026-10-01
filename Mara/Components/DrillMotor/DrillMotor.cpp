// ======================================================================
// \title  DrillMotor.cpp
// \author john
// \brief  cpp file for DrillMotor component implementation class
// ======================================================================

#include "Mara/Components/DrillMotor/DrillMotor.hpp"
#include <Fw/Types/String.hpp>

namespace Mara {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

DrillMotor ::DrillMotor(const char* const compName) : DrillMotorComponentBase(compName) {}
 
DrillMotor ::~DrillMotor() {}

void DrillMotor ::readAndReportStatus() {
    FW_ASSERT(this->isConnected_statusPinRead_OutputPort(0));
 
    Fw::Logic pinState{Fw::Logic::LOW};
    Drv::GpioStatus status = this->statusPinRead_out(0, pinState);
 
    if (status != Drv::GpioStatus::OP_OK) {
        this->log_WARNING_HI_GpioError(Fw::String("READ_STATUS"), static_cast<I32>(status.e));
        return;
    }
 
    const bool state = (pinState == Fw::Logic::HIGH);
    this->tlmWrite_StatusPinState(state);
    this->log_ACTIVITY_LO_StatusPinRead(state);
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
    this->readAndReportStatus();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------
 
void DrillMotor ::schedIn_handler(FwIndexType portNum, U32 context) {
    this->readAndReportStatus();
}
 


}  // namespace Mara
