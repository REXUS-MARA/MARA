// ======================================================================
// \title  ADXL345Manager.cpp
// \author imanol
// \brief  cpp file for ADXL345Manager component implementation class
// ======================================================================

#include "Mara/Components/ADXL345Manager/ADXL345Manager.hpp"
#include "Fw/Types/BasicTypes.hpp"

#define ADXL345_REG_DEVID       0x00
#define ADXL345_REG_BW_RATE     0x2C
#define ADXL345_REG_POWER_CTL   0x2D
#define ADXL345_REG_DATA_FORMAT 0x31
#define ADXL345_REG_DATAX0      0x32
#define ADXL345_DEVICE_ID       0xE5

namespace Mara {

ADXL345Manager::ADXL345Manager(const char* const compName)
    : ADXL345ManagerComponentBase(compName),
      m_initialized(false)
{
    U8 devId = 0;
    Drv::I2cStatus status = this->readRegisters(ADXL345_REG_DEVID, &devId, 1);

    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_ADXL345_INIT_FAILED(static_cast<I32>(status));
        return;
    }

    if (devId != ADXL345_DEVICE_ID) {
        this->log_WARNING_HI_ADXL345_BAD_DEVICE_ID(devId);
        return;
    }

    Fw::ParamValid valid;
    U8 range = this->paramGet_RANGE(valid);
    if (valid != Fw::ParamValid::VALID && valid != Fw::ParamValid::DEFAULT) {
        range = 0;
    }

    status = this->writeRegister(ADXL345_REG_DATA_FORMAT, range & 0x03);

    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_ADXL345_INIT_FAILED(static_cast<I32>(status));
        return;
    }

    status = this->writeRegister(ADXL345_REG_POWER_CTL, 0x08);

    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_ADXL345_INIT_FAILED(static_cast<I32>(status));
        return;
    }

    m_initialized = true;
    this->log_ACTIVITY_HI_ADXL345_INITIALIZED();
}

ADXL345Manager::~ADXL345Manager() {}

// ----------------------------------------------------------------------
// Helper: get I2C address from parameter
// ----------------------------------------------------------------------

U8 ADXL345Manager::getI2cAddr() {
    Fw::ParamValid valid;
    U8 addr = this->paramGet_I2C_ADDR(valid);
    // Use default if parameter not set
    if (valid != Fw::ParamValid::VALID && valid != Fw::ParamValid::DEFAULT) {
        addr = 0x53;
    }
    return addr;
}

// ----------------------------------------------------------------------
// I2C Helpers
// ----------------------------------------------------------------------

Drv::I2cStatus ADXL345Manager::writeRegister(U8 reg, U8 value) {
    U8 data[2] = {reg, value};
    Fw::Buffer buffer(data, sizeof(data));
    return this->i2cReadWrite_out(0, this->getI2cAddr(), buffer);
}

Drv::I2cStatus ADXL345Manager::readRegisters(U8 startReg, U8* outBuffer, U32 size) {
    // Write the register address
    Fw::Buffer regBuf(&startReg, 1);
    Drv::I2cStatus status = this->i2cReadWrite_out(0, this->getI2cAddr(), regBuf);
    if (status != Drv::I2cStatus::I2C_OK) {
        return status;
    }
    // Read back the data using the same port
    Fw::Buffer readBuf(outBuffer, size);
    return this->i2cReadWrite_out(0, this->getI2cAddr(), readBuf);
}

// ----------------------------------------------------------------------
// Command Handlers
// ----------------------------------------------------------------------

void ADXL345Manager::ADXL345_SET_RANGE_cmdHandler(
    FwOpcodeType opCode,
    U32 cmdSeq,
    U8 range
) {
    Drv::I2cStatus status = this->writeRegister(ADXL345_REG_DATA_FORMAT, range & 0x03);
    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_ADXL345_I2C_ERROR(static_cast<I32>(status));
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void ADXL345Manager::ADXL345_SET_RATE_cmdHandler(
    FwOpcodeType opCode,
    U32 cmdSeq,
    U8 rate
) {
    Drv::I2cStatus status = this->writeRegister(ADXL345_REG_BW_RATE, rate);
    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_ADXL345_I2C_ERROR(static_cast<I32>(status));
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Schedule port handler
// ----------------------------------------------------------------------

void ADXL345Manager::run_handler(
    FwIndexType portNum,
    U32 context
) {
    if (!m_initialized) {
        return;
    }

    U8 data[6] = {0};
    Drv::I2cStatus status = this->readRegisters(ADXL345_REG_DATAX0, data, 6);

    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_ADXL345_I2C_ERROR(static_cast<I32>(status));
        return;
    }

    I16 accelX = static_cast<I16>((data[1] << 8) | data[0]);
    I16 accelY = static_cast<I16>((data[3] << 8) | data[2]);
    I16 accelZ = static_cast<I16>((data[5] << 8) | data[4]);

    this->tlmWrite_accelX(accelX);
    this->tlmWrite_accelY(accelY);
    this->tlmWrite_accelZ(accelZ);
}

}  // namespace Mara