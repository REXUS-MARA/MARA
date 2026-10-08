// ======================================================================
// \title  Bmp280Manager.cpp
// \author fsowa
// \brief  cpp file for Bmp280Manager component implementation class
// ======================================================================

#include "Mara/Components/Bmp280Manager/Bmp280Manager.hpp"

namespace Mara {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

Bmp280Manager ::Bmp280Manager(const char* const compName)
    : Bmp280ManagerComponentBase(compName),
      m_dev(),
      m_address(0x77),
      m_lastI2cStatus(Drv::I2cStatus::I2C_OK),
      m_config(),
      m_container(),
      m_containerValid(false),
      m_count(0) {
    this->m_dev.intf = BMP2_I2C_INTF;
    this->m_dev.intf_ptr = this;
    this->m_dev.read = &Bmp280Manager::i2cRead;
    this->m_dev.write = &Bmp280Manager::i2cWrite;
    this->m_dev.delay_us = &Bmp280Manager::delayUs;
}

Bmp280Manager ::~Bmp280Manager() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void Bmp280Manager ::run_handler(FwIndexType portNum, U32 context) {
    this->sensorSm_sendSignal_tick();
    // Queued component: the tick and the success/error signals queued by the
    // previous action are handled here, on the rate group thread
    this->dispatchCurrentMessages();
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void Bmp280Manager ::RESET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // Reuse error to force a reset
    this->sensorSm_sendSignal_error();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void Bmp280Manager ::Mara_I2CSensorStateMachine_action_doReset(SmId smId, Mara_I2CSensorStateMachine::Signal signal) {
    FW_ASSERT(smId == SmId::sensorSm, static_cast<FwAssertArgType>(smId));
    Fw::ParamValid valid;
    this->m_address = this->paramGet_I2C_ADDRESS(valid);
    // Writes 0xB6 to the reset register and waits 2 ms
    this->signalResult(bmp2_soft_reset(&this->m_dev));
}

void Bmp280Manager ::Mara_I2CSensorStateMachine_action_checkReset(SmId smId,
                                                                  Mara_I2CSensorStateMachine::Signal signal) {
    FW_ASSERT(smId == SmId::sensorSm, static_cast<FwAssertArgType>(smId));
    bmp2_status status{};
    const int8_t result = bmp2_get_status(&status, &this->m_dev);
    if (result != BMP2_OK) {
        this->reportBoschFailure(result);
        this->sensorSm_sendSignal_error();
    } else if (status.im_update == 0) {
        // The NVM trimming data has been copied to the image registers
        this->sensorSm_sendSignal_success();
    }
    // Otherwise the copy is still running: check again on the next tick
}

void Bmp280Manager ::Mara_I2CSensorStateMachine_action_doEnable(SmId smId, Mara_I2CSensorStateMachine::Signal signal) {
    FW_ASSERT(smId == SmId::sensorSm, static_cast<FwAssertArgType>(smId));
    // Checks the chip ID and reads the calibration into m_dev
    this->signalResult(bmp2_init(&this->m_dev));
}

void Bmp280Manager ::Mara_I2CSensorStateMachine_action_doConfigure(SmId smId,
                                                                   Mara_I2CSensorStateMachine::Signal signal) {
    FW_ASSERT(smId == SmId::sensorSm, static_cast<FwAssertArgType>(smId));
    const Bmp280Config config = this->configFromParams();
    const bmp2_config boschConfig = toBosch(config);
    // Bosch soft-resets the sensor (calibration stays in m_dev), writes ctrl_meas
    // and config, then starts normal mode
    const int8_t result = bmp2_set_power_mode(BMP2_POWERMODE_NORMAL, &boschConfig, &this->m_dev);
    if (result == BMP2_OK) {
        uint32_t measurementTimeUs = 0;
        (void)bmp2_compute_meas_time(&measurementTimeUs, &boschConfig, &this->m_dev);
        this->log_ACTIVITY_HI_Configured(measurementTimeUs);
        // A container holds readings of one configuration: close the open one early
        if (this->m_containerValid && !(config == this->m_config)) {
            this->dpSend(this->m_container);
            this->m_containerValid = false;
        }
        this->m_config = config;
    }
    this->signalResult(result);
}

void Bmp280Manager ::Mara_I2CSensorStateMachine_action_doRead(SmId smId, Mara_I2CSensorStateMachine::Signal signal) {
    FW_ASSERT(smId == SmId::sensorSm, static_cast<FwAssertArgType>(smId));
    bmp2_uncomp_data raw{};
    bmp2_data compensated{};
    const int8_t status = this->readSensor(raw, compensated);

    switch (status) {
        case BMP2_OK:
            this->log_WARNING_LO_OutOfSpec_ThrottleClear();
            this->log_WARNING_LO_BadReading_ThrottleClear();
            this->log_WARNING_HI_I2cError_ThrottleClear();
            break;
        case BMP2_W_MIN_TEMP:
        case BMP2_W_MAX_TEMP:
        case BMP2_W_MIN_PRES:
        case BMP2_W_MAX_PRES:
            // Outside the specified range (e.g. below 300 hPa above ~9 km): Bosch clamps
            // the value, the raw ADC values in the data product are still valid
            this->log_WARNING_LO_OutOfSpec(status);
            break;
        case BMP2_E_UNCOMP_TEMP_RANGE:
        case BMP2_E_UNCOMP_PRESS_RANGE:
        case BMP2_E_UNCOMP_TEMP_AND_PRESS_RANGE:
            // Raw value outside the ADC range: keep the record so the frame is visible
            this->log_WARNING_LO_BadReading(status);
            break;
        default:
            // Bus failure: start over from RESET
            this->reportBoschFailure(status);
            this->sensorSm_sendSignal_error();
            return;
    }

    Bmp280Data data;
    data.set_pressure(static_cast<F32>(compensated.pressure));
    data.set_temperature(static_cast<F32>(compensated.temperature));
    this->tlmWrite_Reading(data);
    this->tlmWrite_Status(status);
    this->recordReading(data, raw, status);
}

// ----------------------------------------------------------------------
// Parameter updates
// ----------------------------------------------------------------------

void Bmp280Manager ::parameterUpdated(FwPrmIdType id) {
    switch (id) {
        case PARAMID_I2C_ADDRESS:
            // A different device: start over (doReset picks up the new address)
            this->sensorSm_sendSignal_error();
            break;
        case PARAMID_OVERSAMPLING:
        case PARAMID_FILTER:
        case PARAMID_STANDBY:
            this->sensorSm_sendSignal_reconfigure();
            break;
        default:
            FW_ASSERT(0, static_cast<FwAssertArgType>(id));
            break;
    }
}

}  // namespace Mara
