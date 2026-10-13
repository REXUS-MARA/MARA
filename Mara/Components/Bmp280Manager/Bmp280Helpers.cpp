// ======================================================================
// \title  Bmp280Helpers.cpp
// \author fsowa
// \brief  Bosch BMP2 API glue and data product helpers for Bmp280Manager
// ======================================================================

#include <algorithm>
#include <array>

#include "Fw/Time/TimeInterval.hpp"
#include "Mara/Components/Bmp280Manager/Bmp280Manager.hpp"
#include "Os/Task.hpp"

namespace Mara {

namespace {
//! Bosch passes intf_ptr back as const void*; it is the component that set it in the constructor
Bmp280Manager& component(const void* intfPtr) {
    FW_ASSERT(intfPtr != nullptr);
    return *static_cast<Bmp280Manager*>(const_cast<void*>(intfPtr));
}

//! Same range check as the (static) Bosch st_check_boundaries
int8_t checkRawRange(const bmp2_uncomp_data& raw) {
    const bool pressureOk = raw.pressure <= static_cast<uint32_t>(BMP2_ST_ADC_P_MAX);
    const bool temperatureOk = (raw.temperature >= BMP2_ST_ADC_T_MIN) && (raw.temperature <= BMP2_ST_ADC_T_MAX);
    if (pressureOk && temperatureOk) {
        return BMP2_OK;
    }
    if (temperatureOk) {
        return BMP2_E_UNCOMP_PRESS_RANGE;
    }
    if (pressureOk) {
        return BMP2_E_UNCOMP_TEMP_RANGE;
    }
    return BMP2_E_UNCOMP_TEMP_AND_PRESS_RANGE;
}
}  // namespace

// ----------------------------------------------------------------------
// Bosch callbacks
// ----------------------------------------------------------------------

BMP2_INTF_RET_TYPE Bmp280Manager::i2cRead(uint8_t regAddr, uint8_t* regData, uint32_t length, const void* intfPtr) {
    Bmp280Manager& self = component(intfPtr);
    Fw::Buffer writeBuffer(&regAddr, sizeof(regAddr));
    Fw::Buffer readBuffer(regData, length);
    self.m_lastI2cStatus = self.busWriteRead_out(0, self.m_address, writeBuffer, readBuffer);
    return (self.m_lastI2cStatus == Drv::I2cStatus::I2C_OK) ? BMP2_INTF_RET_SUCCESS : -1;
}

BMP2_INTF_RET_TYPE Bmp280Manager::i2cWrite(uint8_t regAddr,
                                           const uint8_t* regData,
                                           uint32_t length,
                                           const void* intfPtr) {
    Bmp280Manager& self = component(intfPtr);
    std::array<U8, MAX_WRITE_SIZE> command{};
    if (length + 1 > command.size()) {
        return -1;
    }
    // [first register, value, (register, value)...]: Bosch already interleaved the burst
    command[0] = regAddr;
    std::copy(regData, regData + length, command.begin() + 1);
    Fw::Buffer writeBuffer(command.data(), length + 1);
    self.m_lastI2cStatus = self.busWrite_out(0, self.m_address, writeBuffer);
    return (self.m_lastI2cStatus == Drv::I2cStatus::I2C_OK) ? BMP2_INTF_RET_SUCCESS : -1;
}

void Bmp280Manager::delayUs(uint32_t period, void* intfPtr) {
    (void)Os::Task::delay(Fw::TimeInterval(period / 1000000, period % 1000000));
}

// ----------------------------------------------------------------------
// State machine helpers
// ----------------------------------------------------------------------

void Bmp280Manager::reportBoschFailure(int8_t result) {
    switch (result) {
        case BMP2_E_COM_FAIL:
            this->log_WARNING_HI_I2cError(this->m_address, this->m_lastI2cStatus);
            break;
        case BMP2_E_DEV_NOT_FOUND:
            this->log_WARNING_HI_ChipIdMismatch(this->m_dev.chip_id);
            break;
        default:
            this->log_WARNING_HI_BoschError(result);
            break;
    }
}

void Bmp280Manager::signalResult(int8_t result) {
    if (result == BMP2_OK) {
        this->sensorSm_sendSignal_success();
    } else {
        this->reportBoschFailure(result);
        this->sensorSm_sendSignal_error();
    }
}

Bmp280Config Bmp280Manager::configFromParams() {
    Fw::ParamValid valid;
    const Bmp280OsMode oversampling = this->paramGet_OVERSAMPLING(valid);
    const Bmp280Filter filter = this->paramGet_FILTER(valid);
    const Bmp280Standby standby = this->paramGet_STANDBY(valid);
    return Bmp280Config(oversampling, filter, standby);
}

bmp2_config Bmp280Manager::toBosch(const Bmp280Config& source) {
    bmp2_config config{};

    switch (source.get_oversampling()) {
        case Bmp280OsMode::ULTRA_LOW_POWER:
            config.os_mode = BMP2_OS_MODE_ULTRA_LOW_POWER;
            break;
        case Bmp280OsMode::LOW_POWER:
            config.os_mode = BMP2_OS_MODE_LOW_POWER;
            break;
        case Bmp280OsMode::STANDARD_RESOLUTION:
            config.os_mode = BMP2_OS_MODE_STANDARD_RESOLUTION;
            break;
        case Bmp280OsMode::HIGH_RESOLUTION:
            config.os_mode = BMP2_OS_MODE_HIGH_RESOLUTION;
            break;
        case Bmp280OsMode::ULTRA_HIGH_RESOLUTION:
            config.os_mode = BMP2_OS_MODE_ULTRA_HIGH_RESOLUTION;
            break;
        default:
            FW_ASSERT(0);
            break;
    }

    switch (source.get_filter()) {
        case Bmp280Filter::OFF:
            config.filter = BMP2_FILTER_OFF;
            break;
        case Bmp280Filter::COEFF_2:
            config.filter = BMP2_FILTER_COEFF_2;
            break;
        case Bmp280Filter::COEFF_4:
            config.filter = BMP2_FILTER_COEFF_4;
            break;
        case Bmp280Filter::COEFF_8:
            config.filter = BMP2_FILTER_COEFF_8;
            break;
        case Bmp280Filter::COEFF_16:
            config.filter = BMP2_FILTER_COEFF_16;
            break;
        default:
            FW_ASSERT(0);
            break;
    }

    switch (source.get_standby()) {
        case Bmp280Standby::MS_0_5:
            config.odr = BMP2_ODR_0_5_MS;
            break;
        case Bmp280Standby::MS_62_5:
            config.odr = BMP2_ODR_62_5_MS;
            break;
        case Bmp280Standby::MS_125:
            config.odr = BMP2_ODR_125_MS;
            break;
        case Bmp280Standby::MS_250:
            config.odr = BMP2_ODR_250_MS;
            break;
        case Bmp280Standby::MS_500:
            config.odr = BMP2_ODR_500_MS;
            break;
        case Bmp280Standby::MS_1000:
            config.odr = BMP2_ODR_1000_MS;
            break;
        case Bmp280Standby::MS_2000:
            config.odr = BMP2_ODR_2000_MS;
            break;
        case Bmp280Standby::MS_4000:
            config.odr = BMP2_ODR_4000_MS;
            break;
        default:
            FW_ASSERT(0);
            break;
    }

    config.spi3w_en = BMP2_SPI3_WIRE_DISABLE;
    return config;
}

int8_t Bmp280Manager::readSensor(bmp2_uncomp_data& raw, bmp2_data& compensated) {
    // In normal mode the data registers are shadowed, so one burst read is always a
    // consistent pair; there is no need to wait for the measuring bit
    std::array<uint8_t, DATA_REGISTERS_SIZE> registers{};
    const int8_t result = bmp2_get_regs(BMP2_REG_PRES_MSB, registers.data(), registers.size(), &this->m_dev);
    if (result != BMP2_OK) {
        return result;
    }

    // 20-bit values: msb[19:12] lsb[11:4] xlsb[7:4] (datasheet section 4.3.6/4.3.7).
    // Parsed here because Bosch's parse_sensor_data is static and the raw values go into the data product.
    raw.pressure = (static_cast<uint32_t>(registers[0]) << 12) | (static_cast<uint32_t>(registers[1]) << 4) |
                   (static_cast<uint32_t>(registers[2]) >> 4);
    raw.temperature = static_cast<int32_t>((static_cast<uint32_t>(registers[3]) << 12) |
                                           (static_cast<uint32_t>(registers[4]) << 4) |
                                           (static_cast<uint32_t>(registers[5]) >> 4));

    const int8_t rangeResult = checkRawRange(raw);
    if (rangeResult != BMP2_OK) {
        return rangeResult;
    }
    return bmp2_compensate_data(&raw, &compensated, &this->m_dev);
}

// ----------------------------------------------------------------------
// Data products
// ----------------------------------------------------------------------

Bmp280Calib Bmp280Manager::calibration() const {
    const bmp2_calib_param& c = this->m_dev.calib_param;
    return Bmp280Calib(c.dig_t1, c.dig_t2, c.dig_t3, c.dig_p1, c.dig_p2, c.dig_p3, c.dig_p4, c.dig_p5, c.dig_p6,
                       c.dig_p7, c.dig_p8, c.dig_p9);
}

void Bmp280Manager::recordReading(const Bmp280Data& data, const bmp2_uncomp_data& raw, I8 status) {
    if (not this->m_containerValid) {
        const Fw::Success dpStatus = this->dpGet_ReadingContainer(CONTAINER_DATA_SIZE, this->m_container);
        if (dpStatus != Fw::Success::SUCCESS) {
            this->log_WARNING_HI_DpMemoryFailure(CONTAINER_DATA_SIZE);
            return;
        }
        this->m_containerValid = true;
        this->m_count = 0;
        this->m_container.setTimeTag(this->getTime());
        this->log_WARNING_HI_DpMemoryFailure_ThrottleClear();
        // Every container starts with the calibration and configuration so it can be decoded on its own
        const Fw::SerializeStatus calibStatus = this->m_container.serializeRecord_CalibRecord(this->calibration());
        FW_ASSERT(calibStatus == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(calibStatus));
        const Fw::SerializeStatus configStatus = this->m_container.serializeRecord_ConfigRecord(this->m_config);
        FW_ASSERT(configStatus == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(configStatus));
    }

    const Fw::Time now = this->getTime();
    Bmp280DataTimed record;
    record.set_time_stamp(Fw::TimeValue(now.getTimeBase(), now.getContext(), now.getSeconds(), now.getUSeconds()));
    record.set_data(data);
    record.set_rawPressure(raw.pressure);
    record.set_rawTemperature(static_cast<U32>(raw.temperature));
    record.set_status(status);

    const Fw::SerializeStatus serializeStatus = this->m_container.serializeRecord_ReadingRecord(record);
    FW_ASSERT(serializeStatus == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(serializeStatus));
    this->m_count += 1;

    if (this->m_count == RECORD_COUNT) {
        this->dpSend(this->m_container);
        this->m_count = 0;
        this->m_containerValid = false;
    }
}

}  // namespace Mara
