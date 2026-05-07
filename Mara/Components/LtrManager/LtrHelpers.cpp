// ======================================================================
// \title  ImuHelpers.cpp
// \author mstarch
// \brief  cpp file for ImuManager component helper function implementations
// ======================================================================

#include "Mara/Components/LtrManager/LtrManager.hpp"
#include "Mara/Components/LtrManager/LtrTypes.hpp"

namespace Mara {

Drv::I2cStatus LtrManager::reset() {
    U8 reset_sequence[] = {Ltr::ALS_CONTR_REGISTER, Ltr::RESET_VALUE_MASK};
    Fw::Buffer writeBuffer(reset_sequence, sizeof(reset_sequence));
    Fw::Buffer readBuffer;
    return bus_write(writeBuffer, readBuffer);
}

Drv::I2cStatus LtrManager::read_ALS_CONTR(U8& value) {
    U8 als_control_register = Ltr::ALS_CONTR_REGISTER;
    Fw::Buffer writeBuffer(&als_control_register, sizeof(als_control_register));
    Fw::Buffer readBuffer(&value, sizeof(value));
    return bus_write(writeBuffer, readBuffer);
}

Drv::I2cStatus LtrManager::read(LtrData& LtrData) {
    U8 data[Ltr::DATA_LEN_CHANNEL];
    U8 registerAddress = Ltr::ALS_DATA_CH1_0_REGISTER;

    Fw::Buffer writeBuffer(&registerAddress, sizeof(registerAddress));
    Fw::Buffer readBuffer(data, Ltr::DATA_LEN_CHANNEL);
    // If bus write fails, state machine is reset, so just return
    Drv::I2cStatus status = this->bus_write(writeBuffer, readBuffer);
    if (status != Drv::I2cStatus::I2C_OK) {
        return status;
    }
    Ltr::RawLtrData raw = this->deserialize_raw_data(readBuffer);

    // Now, we need to get the gain to pass to the convert_raw_data
    // Because we need it to make sense of the reading
    // I'm following the imu implemetation here in reading out the parameter
    // Safer choice I think would be to read the gain from the device
    // and double check here to make sure that those values are the same
    // but I'll only leave thise comment instead of doing that (cause it's also slower)
    // I'm not sure how much tho
    
    Fw::ParamValid paramValid;
    const LtrGain gain = this->paramGet_GAIN(paramValid);
    FW_ASSERT(paramValid != Fw::ParamValid::INVALID, static_cast<FwAssertArgType>(paramValid));

    LtrData = convert_raw_data(raw, gain);
    return status;
}

Ltr::RawLtrData LtrManager::deserialize_raw_data(Fw::Buffer& buffer) {
    auto deserializer = buffer.getDeserializer();
    Ltr::RawLtrData raw;
    // Very important to set correct Endiannes!
    // Look in your data sheet on how it's done
    deserializer.deserializeTo(raw.ch_1, Fw::Endianness::LITTLE);
    deserializer.deserializeTo(raw.ch_0, Fw::Endianness::LITTLE);
    return raw;
}

LtrData LtrManager::convert_raw_data(Ltr::RawLtrData& rawData, [[maybe_unused]]const LtrGain& gain) {
    LtrData ltrData;
    ltrData.set_channel_1(rawData.ch_1);
    ltrData.set_channel_0(rawData.ch_0);
    // leaving it as 0 for now
    // since the formula is quite complicated. And it seems we don't care anyway?
    // we just want to estimate the RPM from it
    U16 lux{0};
    ltrData.set_calculated_lux(lux);
    return ltrData;
}

Drv::I2cStatus LtrManager::bus_write(Fw::Buffer& writeBuffer, Fw::Buffer& readBuffer) {
    Drv::I2cStatus status;
    FW_ASSERT(writeBuffer.isValid());
    if (readBuffer.isValid()) {
        status = this->busWriteRead_out(0, this->m_address, writeBuffer, readBuffer);
    } else {
        status = this->busWrite_out(0, this->m_address, writeBuffer);
    }
    return status;
}

Drv::I2cStatus LtrManager::configure_device() {
    Fw::ParamValid paramValid;
    Drv::I2cStatus status = Drv::I2cStatus::I2C_OK;
    const LtrGain ltrGain = this->paramGet_GAIN(paramValid);
    FW_ASSERT(paramValid != Fw::ParamValid::INVALID, static_cast<FwAssertArgType>(paramValid));
    U8 register_gain = this->gain_to_register(ltrGain);
    // here we set the gain and we also tell the ltr to be active
    U8 control_value = register_gain | Ltr::ALS_ACTIVE_MODE_MASK;
    U8 control_sequence[] = {Ltr::ALS_CONTR_REGISTER, control_value};
    Fw::Buffer writeBuffer(control_sequence, sizeof(control_sequence));
    Fw::Buffer readBuffer;
    status = this->bus_write(writeBuffer, readBuffer);

    if (status != Drv::I2cStatus::I2C_OK) {
        return status;
    }

    // and now, we should set the measurement rate to the quickest one
    U8 measurement_value = Ltr::ALS_INTEGRATION_TIME_50MS | Ltr::ALS_MEASUREMENT_RATE_50MS;
    U8 measurement_sequence[] = {Ltr::ALS_MEAS_RATE, measurement_value};
    
    Fw::Buffer writeBuffer_measure(measurement_sequence, sizeof(measurement_sequence));
    Fw::Buffer readBuffer_measure;
    status = this->bus_write(writeBuffer_measure, readBuffer_measure);

    return status;
}

U8 LtrManager::gain_to_register(LtrGain gain){
    U8 registerValue = 0;
    switch (gain.e) {
        case LtrGain::GAIN_1X:
            registerValue = Ltr::GAIN_CONFIG_1X;
            break;
        case LtrGain::GAIN_2X:
            registerValue = Ltr::GAIN_CONFIG_2X;
            break;
        case LtrGain::GAIN_4X:
            registerValue = Ltr::GAIN_CONFIG_4X;
            break;
        case LtrGain::GAIN_8X:
            registerValue = Ltr::GAIN_CONFIG_8X;
            break;
        case LtrGain::GAIN_48X:
            registerValue = Ltr::GAIN_CONFIG_48X;
            break;
        case LtrGain::GAIN_96X:
            registerValue = Ltr::GAIN_CONFIG_96X;
            break;
        default:
            FW_ASSERT(0, gain.e);
            break;
    }
    return registerValue;
}


}  // namespace Mara