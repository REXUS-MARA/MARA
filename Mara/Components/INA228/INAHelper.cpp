// ======================================================================
// \title  INAHelper.cpp
// \author aurahs
// \brief  helper functions for INA228
// ======================================================================


Drv::I2cStatus INA228::reset() {
    /*
     Reset the device
     TODO: reset the device
    */
    Drv::I2cStatus status = Drv::I2cStatus::I2C_OK;
    return status;
}

Drv::I2cStatus INA228::configure_device() {
    /*
     Configure current calibration register
     The rest of the standard configuration is sufficient :+1:
    */
    Drv::I2cStatus status = Drv::I2cStatus::I2C_OK;
    status = this->write_register(INA::SHUNT_CAL, INA::SHUNT_CAL_VAL);
    return status;
}



Drv::I2cStatus INA228::write_register(U8 registerAddress, U16 value) {
    /*
     Write a single configuration register
    */
    Drv::I2cStatus status;
    // Write commands are on the form [address, MSB, LSB]
    U8 command[INA::COMMANDSIZE] = {registerAddress, (U8) (value >> 8), (U8) value};
    Fw::Buffer writeBuffer(command, INA::COMMANDSIZE);
    status = this->busWrite_out(0, this->m_address, writeBuffer);
    return status;
}

Drv::I2cStatus INA288::read_register(U8 registerAddress, I32& value) {
    /*
     Read from a single register and deserializes the value
    */
    U8 rawData[INA::DATASIZE];
    Fw::Buffer writeBuffer(registerAddress, 1);
    Fw::Buffer readBuffer(rawData, INA::DATA_SIZE);
    Drv::I2cStatus status = this->busWriteRead_out(0, m_address, writeBuffer, readBuffer);
    if (status == Drv::I2cStatus::I2C_OK) {
        auto deserializer = readBuffer.getDeserializer();
        deserializer.deserializeTo(value, Fw::Endianness::BIG);
    }
    return status;

}


Drv::I2cStatus INA228::read(INAData& data) {
    /*
     Read voltage and current data from the sensor
    */
    Drv::I2cStatus status = Drv::I2cStatus::I2C_OK;
    status = read_register(INA::VBUS, data->voltage);
    if (status == Drv::I2cStatus::I2C_OK) {
        status = read_register(INA::CURRENT, data->current);
    }
    return status;
}


