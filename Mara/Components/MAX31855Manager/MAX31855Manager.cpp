// ======================================================================
// \title  MAX31855Manager.cpp
// \author John
// \brief  cpp file for MAX31855Manager component implementation class
// ======================================================================

#include "Mara/Components/MAX31855Manager/MAX31855Manager.hpp"
#include "Fw/Types/BasicTypes.hpp"

// MAX31855 32-bit frame layout (MSB first):
//   D31..D18  14-bit signed thermocouple temperature, 0.25 C/LSB
//   D17       reserved, always 0
//   D16       fault (set if any of D2..D0 is set)
//   D15..D4   12-bit signed internal temperature, 0.0625 C/LSB
//   D3        reserved, always 0
//   D2        SCV: thermocouple shorted to VCC
//   D1        SCG: thermocouple shorted to GND
//   D0        OC:  thermocouple open circuit
#define MAX31855_FRAME_LEN        4
#define MAX31855_RESERVED_MASK    0x00020008U
#define MAX31855_FAULT_BIT        0x00010000U
#define MAX31855_FAULT_MASK       0x07U
#define MAX31855_TC_C_PER_LSB     0.25f
#define MAX31855_INT_C_PER_LSB    0.0625f

namespace Mara {

MAX31855Manager::MAX31855Manager(const char* const compName)
    : MAX31855ManagerComponentBase(compName),
      m_count(0),
      m_container(),
      m_containerValid(false),
      m_initialized(false),
      m_lastFaults(0)
{}

MAX31855Manager::~MAX31855Manager() {}

// ----------------------------------------------------------------------
// SPI Helpers
// ----------------------------------------------------------------------

Drv::SpiStatus MAX31855Manager::readFrame(U32& frame) {
    //The driver doesn't have MOSI so we bullshit to make them equal
    U8 txData[MAX31855_FRAME_LEN] = {0, 0, 0, 0};
    U8 rxData[MAX31855_FRAME_LEN] = {0, 0, 0, 0};
    Fw::Buffer writeBuffer(txData, sizeof(txData));
    Fw::Buffer readBuffer(rxData, sizeof(rxData));

    Drv::SpiStatus status = this->spiWriteRead_out(0, writeBuffer, readBuffer);
    if (status != Drv::SpiStatus::SPI_OK) {
        return status;
    }

    auto deserializer = readBuffer.getDeserializer();
    Fw::SerializeStatus ser_status = deserializer.deserializeTo(frame, Fw::Endianness::BIG);
    FW_ASSERT(ser_status == Fw::SerializeStatus::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(ser_status));
    return status;
}

bool MAX31855Manager::frameValid(U32 frame) {
    //Check if the frame is valid
    return (frame & MAX31855_RESERVED_MASK) == 0;
}

ThermoData MAX31855Manager::decodeFrame(U32 frame) {
    // Thermocouple: 14-bit two's complement in D31..D18
    U16 tcBits = static_cast<U16>((frame >> 18) & 0x3FFFU);
    if (tcBits & 0x2000U) {
        tcBits |= 0xC000U;  // sign extend to 16 bits
    }
    const I16 tcRaw = static_cast<I16>(tcBits);

    // Internal: 12-bit two's complement in D15..D4
    U16 intBits = static_cast<U16>((frame >> 4) & 0x0FFFU);
    if (intBits & 0x0800U) {
        intBits |= 0xF000U;  // sign extend to 16 bits
    }
    const I16 intRaw = static_cast<I16>(intBits);

    ThermoData data;
    data.set_thermocoupleC(static_cast<F32>(tcRaw) * MAX31855_TC_C_PER_LSB);
    data.set_internalC(static_cast<F32>(intRaw) * MAX31855_INT_C_PER_LSB);
    data.set_faults(static_cast<U8>(frame & MAX31855_FAULT_MASK));
    return data;
}

// ----------------------------------------------------------------------
// Schedule port handler
// ----------------------------------------------------------------------

void MAX31855Manager::run_handler(
    FwIndexType portNum,
    U32 context
) {
    U32 frame = 0;
    Drv::SpiStatus read_status = this->readFrame(frame);
    if (read_status != Drv::SpiStatus::SPI_OK) {
        this->log_WARNING_HI_MAX31855_SPI_ERROR(static_cast<I32>(read_status.e));
        return;
    }
    this->log_WARNING_HI_MAX31855_SPI_ERROR_ThrottleClear();

    if (!frameValid(frame)) {
        this->log_WARNING_HI_MAX31855_BAD_FRAME(frame);
        return;
    }
    this->log_WARNING_HI_MAX31855_BAD_FRAME_ThrottleClear();

    // The MAX31855 has no initialization frames, so first valid data counts
    if (!m_initialized) {
        m_initialized = true;
        this->log_ACTIVITY_HI_MAX31855_INITIALIZED();
    }

    ThermoData thermoData = decodeFrame(frame);

    // Log faults on change only, so it isn't like the adxl and spams events when disconnected
    const U8 faults = thermoData.get_faults();
    if (faults != m_lastFaults) {
        if (faults != 0) {
            this->log_WARNING_HI_MAX31855_FAULT(faults);
        } else {
            this->log_ACTIVITY_HI_MAX31855_FAULT_CLEARED();
        }
        m_lastFaults = faults;
    }

    this->tlmWrite_thermocoupleTemp(thermoData.get_thermocoupleC());
    this->tlmWrite_internalTemp(thermoData.get_internalC());
    this->tlmWrite_faults(faults);
    this->tlmWrite_temperature(thermoData);

    if (not this->m_containerValid) {
        const FwSizeType containerSize =
            RECORD_COUNT * (ThermoDataTimed::SERIALIZED_SIZE + sizeof(FwDpIdType));

        // Initialize the data product container
        Fw::Success dp_status = dpGet_ThermoContainer(containerSize, this->m_container);
        if (dp_status != Fw::Success::SUCCESS) {
            this->log_WARNING_HI_DpMemoryFailure(containerSize);
        } else {
            this->m_containerValid = true;
            this->m_count = 0;
            this->m_container.setTimeTag(this->getTime());
            this->log_WARNING_HI_DpMemoryFailure_ThrottleClear();
        }
    }

    if (this->m_containerValid) {
        Fw::Time currentFwTime = this->getTime();
        Fw::TimeValue currentTime = Fw::TimeValue(currentFwTime.getTimeBase(), currentFwTime.getContext(),
                                                  currentFwTime.getSeconds(), currentFwTime.getUSeconds());
        ThermoDataTimed thermoDataTimed;
        thermoDataTimed.set_time_stamp(currentTime);
        thermoDataTimed.set_data(thermoData);

        Fw::SerializeStatus serialize_status = m_container.serializeRecord_ThermoRecord(thermoDataTimed);
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
