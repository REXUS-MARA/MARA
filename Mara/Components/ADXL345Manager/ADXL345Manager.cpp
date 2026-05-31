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
      m_containerValid(false),
      m_range(0)
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
    const size_t devIdLen = 1;
    U8 data[devIdLen];
    Fw::Buffer readBuffer(data, devIdLen);
    Drv::I2cStatus status = this->readRegisters(ADXL345_REG_DEVID, readBuffer);

    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_ADXL345_INIT_FAILED(static_cast<I32>(status));
        return status;
    }

    U8 devId = 0;
    readBuffer.getDeserializer().deserializeTo(devId);
    if (devId != ADXL345_DEVICE_ID) {
        this->log_WARNING_HI_ADXL345_BAD_DEVICE_ID(devId);
        return status;
    }

    Fw::ParamValid valid;
    U8 range = this->paramGet_RANGE(valid);
    if (valid != Fw::ParamValid::VALID && valid != Fw::ParamValid::DEFAULT) {
        range = 0;
    }

    m_range = range & 0x03;

    status = this->writeRegister(ADXL345_REG_DATA_FORMAT, m_range);

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
    return this->i2cWrite_out(0, this->getI2cAddr(), writeBuffer);
}

Drv::I2cStatus ADXL345Manager::readRegisters(U8 startReg, Fw::Buffer& readBuffer) {
    Fw::Buffer writeBuffer(&startReg, 1);
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
    m_range = range & 0x03;
    Drv::I2cStatus status = this->writeRegister(ADXL345_REG_DATA_FORMAT, m_range);
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

// Scale factors in g per LSB for each range setting
// Range 0 = ±2g  -> 3.9 mg/LSB
// Range 1 = ±4g  -> 7.8 mg/LSB
// Range 2 = ±8g  -> 15.6 mg/LSB
// Range 3 = ±16g -> 31.2 mg/LSB
static const F32 SCALE_FACTORS_G_PER_LSB[] = {3.9e-3f, 7.8e-3f, 15.6e-3f, 31.2e-3f};

F32 ADXL345Manager::getScaleFactor() {
    return SCALE_FACTORS_G_PER_LSB[m_range & 0x03];
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

    const size_t accelDataLen = 6;
    U8 data[accelDataLen];
    Fw::Buffer readBuffer(data, accelDataLen);;
    Drv::I2cStatus read_status = this->readRegisters(ADXL345_REG_DATAX0, readBuffer);

    if (read_status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_ADXL345_I2C_ERROR(static_cast<I32>(read_status));
        return;
    }

    // Combine bytes into signed 16-bit values (little-endian from sensor)
    auto deserializer = readBuffer.getDeserializer();
    I16 rawX, rawY, rawZ;
    deserializer.deserializeTo(rawX, Fw::Endianness::LITTLE);
    deserializer.deserializeTo(rawY, Fw::Endianness::LITTLE);
    deserializer.deserializeTo(rawZ, Fw::Endianness::LITTLE);

    // Convert raw LSB counts to g values
    F32 scaleFactor = this->getScaleFactor();
    AccelData accelData;
    accelData.set_accelX(static_cast<F32>(rawX) * scaleFactor);
    accelData.set_accelY(static_cast<F32>(rawY) * scaleFactor);
    accelData.set_accelZ(static_cast<F32>(rawZ) * scaleFactor);
    this->tlmWrite_accelX(accelData.get_accelX());
    this->tlmWrite_accelY(accelData.get_accelY());
    this->tlmWrite_accelZ(accelData.get_accelZ());
    this->tlmWrite_acceleration(accelData);
    
    if (not this->m_containerValid) {
        
        const FwSizeType containerSize = RECORD_COUNT * (AccelDataTimed::SERIALIZED_SIZE + sizeof(FwDpIdType));

        // Initialize the data product container
        Fw::Success dp_status = dpGet_AccelContainer(containerSize, this->m_container);
        if (dp_status != Fw::Success::SUCCESS) {
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