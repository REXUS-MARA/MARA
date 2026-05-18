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
      m_initialized(false),
      m_count(0),
      m_container(),
      m_containerValid(false)
{}

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

Drv::I2cStatus ADXL345Manager::initialize_helper(){
    U8 devId = 0;
    Drv::I2cStatus status = this->readRegisters(ADXL345_REG_DEVID, &devId, 1);

    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_ADXL345_INIT_FAILED(static_cast<I32>(status));
        return status;
    }

    if (devId != ADXL345_DEVICE_ID) {
        this->log_WARNING_HI_ADXL345_BAD_DEVICE_ID(devId);
        return status;
    }

    Fw::ParamValid valid;
    U8 range = this->paramGet_RANGE(valid);
    if (valid != Fw::ParamValid::VALID && valid != Fw::ParamValid::DEFAULT) {
        range = 0;
    }

    status = this->writeRegister(ADXL345_REG_DATA_FORMAT, range & 0x03);

    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_ADXL345_INIT_FAILED(static_cast<I32>(status));
        return status;
    }

    status = this->writeRegister(ADXL345_REG_POWER_CTL, 0x08);

    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_ADXL345_INIT_FAILED(static_cast<I32>(status));
        return status;
    }

    m_initialized = true;
    this->log_ACTIVITY_HI_ADXL345_INITIALIZED();
    return status;
}


// ----------------------------------------------------------------------
// I2C Helpers
// ----------------------------------------------------------------------

Drv::I2cStatus ADXL345Manager::writeRegister(U8 reg, U8 value) {
    U8 writeData[2] = {reg, value};
    Fw::Buffer writeBuffer(writeData, sizeof(writeData));
    Fw::Buffer readBuffer;
    return this->i2cReadWrite_out(0, this->getI2cAddr(), writeBuffer, readBuffer);
}

Drv::I2cStatus ADXL345Manager::readRegisters(U8 startReg, U8* outBuffer, U32 size) {
    Fw::Buffer writeBuffer(&startReg, 1);
    Fw::Buffer readBuffer(outBuffer, size);
    return this->i2cReadWrite_out(0, this->getI2cAddr(), writeBuffer, readBuffer);
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
        Drv::I2cStatus initialize_status = initialize_helper();
        if(initialize_status != Drv::I2cStatus::I2C_OK){
            this->log_WARNING_HI_ADXL345_I2C_ERROR(static_cast<I32>(initialize_status));
            return;
        }
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

    AccelData accelData;
    
    if (not this->m_containerValid) {
        
        const FwSizeType containerSize = RECORD_COUNT * (AccelDataTimed::SERIALIZED_SIZE + sizeof(FwDpIdType));

        // Initialize the data product container
        Fw::Success status = dpGet_AccelContainer(containerSize, this->m_container);
        if (status != Fw::Success::SUCCESS) {
            this->log_WARNING_HI_DpMemoryFailure(containerSize);
        } else {
            this->m_containerValid = true;
            this->m_container.setTimeTag(this->getTime());
            this->log_WARNING_HI_DpMemoryFailure_ThrottleClear();
        }
    }

    if (this->m_containerValid) {
        Fw::Time currentFwTime = this->getTime();
        Fw::TimeValue currentTime = Fw::TimeValue(currentFwTime.getTimeBase(), currentFwTime.getContext(),
                                                currentFwTime.getSeconds(), currentFwTime.getUSeconds());
        // Calculate sine and cosine records
        AccelDataTimed accelDataTimed;
        accelDataTimed.set_time_stamp(currentTime);
        accelDataTimed.set_data(accelData);

        // Serialize the records into the data product container
        Fw::SerializeStatus serialize_status = m_container.serializeRecord_AccelRecord(accelDataTimed);
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

}  // namespace Mara