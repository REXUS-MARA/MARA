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

Drv::I2cStatus LtrManager::enable() {
    U8 active_sequence[] = {Ltr::ALS_CONTR_REGISTER, Ltr::ALS_ACTIVE_MODE_MASK};
    Fw::Buffer writeBuffer(active_sequence, sizeof(active_sequence));
    Fw::Buffer readBuffer;
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

    // To Do
    // We need to get the current gain, and then pass that into convert_raw_data
    // That will be done together with adding the configurating

    LtrData = convert_raw_data(raw);
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

LtrData LtrManager::convert_raw_data(Ltr::RawLtrData& rawData) {
    LtrData ltrData;
    ltrData.set_channel_1(rawData.ch_1);
    ltrData.set_channel_0(rawData.ch_0);
    // U16 lux;
    // ltrData.set_calculated_lux(lux);
    // above will be done when I'll add support for configuration
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

}  // namespace Mara