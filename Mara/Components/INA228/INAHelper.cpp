// ======================================================================
// \title  INAHelper.cpp
// \author aurahs
// \brief  helper functions for INA228
// ======================================================================

#include "Mara/Components/INA228/INA228.hpp"
#include "Mara/Components/INA228/INATypes.hpp"

namespace Mara {

Drv::I2cStatus INA228::reset() {
    /*
     Reset the device
    */
    Drv::I2cStatus status = this->write_register(INA::CONFIG, (U16) NULL);
    return status;
}

Drv::I2cStatus INA228::check_reset() {
    /*
     Check if the device was reset by checking the SHUNT_CAL register
     the INA-228 doesn't seem to have any explicit reset reporting otherwise
    */
    I32 cal_value = 0;
    Drv::I2cStatus status = this->read_register(INA::SHUNT_CAL, &cal_value);
    if (status != Drv::I2cStatus::I2C_OK) {
        return status;
    }
    if ((I32) 0x1000 == cal_value) {
        status = Drv::I2cStatus::I2C_OK;
    }
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

Drv::I2cStatus INA228::read_register(U8 registerAddress, I32* value) {
    /*
     Read from a single register and deserializes the value
    */
    U8 rawData[INA::DATASIZE];
    U32 tempValue = 0;
    Fw::Buffer writeBuffer(&registerAddress, 1);
    Fw::Buffer readBuffer(rawData, INA::DATASIZE);
    Drv::I2cStatus status = this->busWriteRead_out(0, this->m_address, writeBuffer, readBuffer);
    if (status == Drv::I2cStatus::I2C_OK) {
        // i really wish the deserializer worked, but it doesnt seem to
        // the value is 20 bits, hence the weird values
        tempValue += readBuffer.getData()[0] << 12;
        tempValue += readBuffer.getData()[1] << 4;
        tempValue += readBuffer.getData()[2] >> 4;
        *value = tempValue;
    }
    return status;

}


Drv::I2cStatus INA228::read(INAData* data) {
    /*
     Read voltage and current data from the sensor
    */
    I32 int_voltage, int_current;
    Drv::I2cStatus status = Drv::I2cStatus::I2C_OK;
    status = read_register(INA::VBUS, &int_voltage);
    if (status == Drv::I2cStatus::I2C_OK) {
        status = read_register(INA::CURRENT, &int_current);
    }
    data->set_current((F32) int_current * INA::CURRENT_LSB);
    data->set_voltage((F32) int_voltage * INA::VOLTAGE_LSB);
    return status;
}

}
