// ======================================================================
// \title  LtrManager.cpp
// \author fsowa
// \brief  cpp file for LtrManager component implementation class
// ======================================================================

#include "Mara/Components/LtrManager/LtrManager.hpp"
#include "Fw/Logger/Logger.hpp"

namespace Mara {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

LtrManager ::LtrManager(const char* const compName)
    : LtrManagerComponentBase(compName), m_address(Ltr::DEVICE_DEFAULT_ADDRESS) {}

LtrManager ::~LtrManager() {}

void LtrManager::configure(U8 device_address) {
    this->m_address = device_address;
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void LtrManager ::run_handler(FwIndexType portNum, U32 context) {
    this->LtrStateMachine_sendSignal_tick();
    this->dispatchCurrentMessages();
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void LtrManager ::RESET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // Reuse error call to force a reset
    this->LtrStateMachine_sendSignal_error();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void LtrManager ::Mara_LtrStateMachine_action_doReset(SmId smId, Mara_LtrStateMachine::Signal signal) {
    // This function is implemented only for the specific instance "LtrStateMachine"
    FW_ASSERT(smId == SmId::LtrStateMachine);
    // Fw::Logger::log("In the reset state");
    // Above is left as a comment, because this component serves also as a reference
    // for other sensors. That is how you can do logging in F'
    Drv::I2cStatus status = this->reset();
    // Transition to RESET state on failure
    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_I2cError(this->m_address, status);
        this->LtrStateMachine_sendSignal_error();
    } else {
        this->LtrStateMachine_sendSignal_success();
    }
}

void LtrManager ::Mara_LtrStateMachine_action_checkReset(SmId smId, Mara_LtrStateMachine::Signal signal) {
    // This function is implemented only for the specific instance "LtrStateMachine"
    FW_ASSERT(smId == SmId::LtrStateMachine);
    U8 reset_val = 0;
    Drv::I2cStatus status = this->read_ALS_CONTR(reset_val);
    // When reset is complete, the second low bit will be 0
    if ((status != Drv::I2cStatus::I2C_OK)) {
        this->log_WARNING_HI_I2cError(this->m_address, status);
        this->LtrStateMachine_sendSignal_error();
    } else if ((reset_val & Ltr::RESET_VALUE_MASK) == 0) {
        this->LtrStateMachine_sendSignal_success();
    }
}

void LtrManager ::Mara_LtrStateMachine_action_doConfigure(SmId smId, Mara_LtrStateMachine::Signal signal)
{
    // This function is implemented only for the specific instance "ltrStateMachine"
    FW_ASSERT(smId == SmId::LtrStateMachine);
    Drv::I2cStatus status = this->configure_device();
    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_I2cError(this->m_address, status);
        this->LtrStateMachine_sendSignal_error();
    } else {
        this->LtrStateMachine_sendSignal_success();
    }
}

void LtrManager ::Mara_LtrStateMachine_action_doRead(SmId smId, Mara_LtrStateMachine::Signal signal) {
    // This function is implemented only for the specific instance "ltrStateMachine"
    FW_ASSERT(smId == SmId::LtrStateMachine);
    LtrData LtrData;
    Drv::I2cStatus status = this->read(LtrData);
    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_I2cError(this->m_address, status);
        this->LtrStateMachine_sendSignal_error();
    } else {
        this->tlmWrite_Reading(LtrData);
    }
}

void LtrManager::parameterUpdated(FwPrmIdType id) {
    Fw::ParamValid isValid = Fw::ParamValid::INVALID;
    switch (id) {
        case PARAMID_GAIN: {
            // Read back the parameter value
            const LtrGain gain = this->paramGet_GAIN(isValid);
            // NOTE: isValid is always VALID in parameterUpdated as it was just properly set
            FW_ASSERT(isValid == Fw::ParamValid::VALID, static_cast<FwAssertArgType>(isValid));
            this->log_ACTIVITY_HI_GainUpdated(gain);
            this->LtrStateMachine_sendSignal_reconfigure();
            break;
        }
        default:
            FW_ASSERT(0, static_cast<FwAssertArgType>(id));
            break;
    }
}

}  // namespace Mara
