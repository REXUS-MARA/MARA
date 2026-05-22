// ======================================================================
// \title  INA228.cpp
// \author sebahs
// \brief  cpp file for INA228 component implementation class
// ======================================================================

#include "Mara/Components/INA228/INA228.hpp"

namespace Mara {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

INA228 ::INA228(const char* const compName) :
    INA228ComponentBase(compName), m_address(INA::DEVICE_DEFAULT_ADDRESS),
    m_count(0), m_container(), m_containerValid(false) {}

INA228 ::~INA228() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void INA228 ::run_handler(FwIndexType portNum, U32 context) {
    this->INAStateMachine_sendSignal_tick();
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void INA228 ::RESET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // Reuse error call to force a reset
    this->INAStateMachine_sendSignal_error();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void INA228 ::Mara_I2CSensorStateMachine_action_doReset(SmId smId, Mara_I2CSensorStateMachine::Signal signal) {
    Drv::I2cStatus status = this->reset();
    // Transition to RESET state on failure
    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_I2cError(this->m_address, status);
        this->INAStateMachine_sendSignal_error();
    } else {
        this->INAStateMachine_sendSignal_success();
    }
}

void INA228 ::Mara_I2CSensorStateMachine_action_checkReset(SmId smId, Mara_I2CSensorStateMachine::Signal signal) {
//    U8 reset_val = 0;
//    Drv::I2cStatus status = this->read_ALS_CONTR(reset_val);
    Drv::I2cStatus status = Drv::I2cStatus::I2C_OK;
    // When reset is complete, the second low bit will be 0
    if ((status != Drv::I2cStatus::I2C_OK)) {
        this->log_WARNING_HI_I2cError(this->m_address, status);
        this->INAStateMachine_sendSignal_error();
    } else { // TODO: check if the thang was reset
        this->INAStateMachine_sendSignal_success();
    }
}

void INA228 ::Mara_I2CSensorStateMachine_action_doEnable(SmId smId, Mara_I2CSensorStateMachine::Signal signal) {
    // TODO
    // why is this here?
}

void INA228 ::Mara_I2CSensorStateMachine_action_doConfigure(SmId smId, Mara_I2CSensorStateMachine::Signal signal) {
    Drv::I2cStatus status = this->configure_device();
    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_I2cError(this->m_address, status);
        this->INAStateMachine_sendSignal_error();
    } else {
        this->INAStateMachine_sendSignal_success();
    }
}

void INA228 ::Mara_I2CSensorStateMachine_action_doRead(SmId smId, Mara_I2CSensorStateMachine::Signal signal) {

    INAData INAData;
    Drv::I2cStatus status = this->read(INAData);
    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_I2cError(this->m_address, status);
        this->INAStateMachine_sendSignal_error();
    } else {
        this->tlmWrite_Reading(INAData);
        // and now, we can also deal with our data products
        // i mostly copied that form this example - https://github.com/nasa/fprime-examples/blob/devel/FlightExamples/DataProduct/Producer/Producer.cpp
        if (not m_containerValid) {
            // Record count * size of each record * 2 record types
            const FwSizeType containerSize = RECORD_COUNT * (INADataTimed::SERIALIZED_SIZE + sizeof(FwDpIdType));

            // Initialize the data product container
            Fw::Success status = dpGet_INAContainer(containerSize, this->m_container);
            if (status != Fw::Success::SUCCESS) {
                this->log_WARNING_HI_DpMemoryFailure(containerSize);
            } else {
                this->m_containerValid = true;
                this->m_container.setTimeTag(this->getTime());
                this->log_WARNING_HI_DpMemoryFailure_ThrottleClear();
            }
        }
        // If we have a valid container, serialize records into it
        // it's not an else, in the correct flow both if cases can be hit
        if (this->m_containerValid) {
            Fw::Time currentFwTime = this->getTime();
            Fw::TimeValue currentTime = Fw::TimeValue(currentFwTime.getTimeBase(), currentFwTime.getContext(),
                                                    currentFwTime.getSeconds(), currentFwTime.getUSeconds());
            // Calculate sine and cosine records
            INADataTimed INADataTimed;
            INADataTimed.set_time_stamp(currentTime);
            INADataTimed.set_data(INAData);

            // Serialize the records into the data product container
            Fw::SerializeStatus serialize_status = m_container.serializeRecord_INARecord(INADataTimed);
            FW_ASSERT(serialize_status == Fw::SerializeStatus::FW_SERIALIZE_OK);
            this->m_count += 1;

            // If we've reached the record count, send the full product
            if (this->m_count == RECORD_COUNT) {
                this->dpSend(this->m_container);
                this->m_count = 0;
                this->m_containerValid = false;
            }
        }
    }
}

}  // namespace Mara
