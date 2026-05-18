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

// Scale factors in g per LSB for each range setting
// Range 0 = ±2g  -> 3.9 mg/LSB
// Range 1 = ±4g  -> 7.8 mg/LSB
// Range 2 = ±8g  -> 15.6 mg/LSB
// Range 3 = ±16g -> 31.2 mg/LSB
static const F32 SCALE_FACTORS_G_PER_LSB[] = {3.9e-3f, 7.8e-3f, 15.6e-3f, 31.2e-3f};

namespace Mara {

ADXL345Manager::ADXL345Manager(const char* const compName)
    : ADXL345ManagerComponentBase(compName),
      m_initialized(false),
      m_range(0),
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
    if (valid != Fw::ParamValid::VALID && valid != Fw::ParamValid::DEFAULT) {
        addr = 0x53;
    }
    return addr;
}

// ----------------------------------------------------------------------
// Helper: get scale factor based on current range
// ----------------------------------------------------------------------

F32 ADXL345Manager::getScaleFactor() {
    return SCALE_FACTORS_G_PER_LSB[m_range & 0x03];
}

Drv::I2cStatus ADXL345Manager::initialize_helper() {
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
    m_range = range & 0x03;
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
        if (initialize_status != Drv::I2cStatus::I2C_OK) {
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

    // Combine bytes into signed 16-bit values (little-endian from sensor)
    I16 rawX = static_cast<I16>(static_cast<U16>(data[1]) << 8 | data[0]);
    I16 rawY = static_cast<I16>(static_cast<U16>(data[3]) << 8 | data[2]);
    I16 rawZ = static_cast<I16>(static_cast<U16>(data[5]) << 8 | data[4]);

    // Convert raw LSB counts to g values
    F32 scaleFactor = this->getScaleFactor();
    F32 accelXg = static_cast<F32>(rawX) * scaleFactor;
    F32 accelYg = static_cast<F32>(rawY) * scaleFactor;
    F32 accelZg = static_cast<F32>(rawZ) * scaleFactor;

    // Write scaled telemetry (in g)
    this->tlmWrite_accelX(accelXg);
    this->tlmWrite_accelY(accelYg);
    this->tlmWrite_accelZ(accelZg);

    // Populate accelData with scaled g values
    AccelData accelData;
    accelData.set_accelX(accelXg);
    accelData.set_accelY(accelYg);
    accelData.set_accelZ(accelZg);

    if (not this->m_containerValid) {
        const FwSizeType containerSize = RECORD_COUNT * (AccelDataTimed::SERIALIZED_SIZE + sizeof(FwDpIdType));

        Fw::Success dpStatus = dpGet_AccelContainer(containerSize, this->m_container);
        if (dpStatus != Fw::Success::SUCCESS) {
            this->log_WARNING_HI_DpMemoryFailure(containerSize);
        } else {
            this->m_containerValid = true;
            this->m_container.setTimeTag(this->getTime());
            this->log_WARNING_HI_DpMemoryFailure_ThrottleClear();
        }
    }

    if (this->m_containerValid) {
        Fw::Time currentFwTime = this->getTime();
        Fw::TimeValue currentTime = Fw::TimeValue(
            currentFwTime.getTimeBase(),
            currentFwTime.getContext(),
            currentFwTime.getSeconds(),
            currentFwTime.getUSeconds()
        );

        AccelDataTimed accelDataTimed;
        accelDataTimed.set_time_stamp(currentTime);
        accelDataTimed.set_data(accelData);

        Fw::SerializeStatus serialize_status = m_container.serializeRecord_AccelRecord(accelDataTimed);
        FW_ASSERT(serialize_status == Fw::SerializeStatus::FW_SERIALIZE_OK);
        this->m_count += 1;

        if (this->m_count == RECORD_COUNT) {
            this->dpSend(this->m_container);
            this->m_count = 0;
            this->m_containerValid = false;
        }
    }
}

}  // namespace Mara