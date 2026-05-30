// ======================================================================
// \title  MAX31855Manager.cpp
// \author john
// \brief  cpp file for MAX31855Manager component implementation class
// ======================================================================

#include "Mara/Components/MAX31855Manager/MAX31855Manager.hpp"

namespace Mara {


static float decodeThermocouple(uint32_t raw)
{
    int16_t value = (raw >> 18) & 0x3FFF;

    // sign extend negative values as the thermocouple gives 14-bit values
    if (value & 0x2000) {
        value |= 0xC000;
    }

    return value * 0.25f;
}

static float decodeColdJunction(uint32_t raw)
{
    int16_t value = (raw >> 4) & 0x0FFF;

    // sign extend 12-bit signed number
    if (value & 0x0800) {
        value |= 0xF000;
    }

    return value * 0.0625f;
}

static uint8_t decodeFaults(uint32_t raw)
{
    if (!(raw & 0x10000)) {
        // no fault present
        return 0;
    }

    return raw & 0x7;
}

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

MAX31855Manager ::MAX31855Manager(const char* const compName) : MAX31855ManagerComponentBase(compName) {}

MAX31855Manager ::~MAX31855Manager() {}

void MAX31855Manager::handleFaults(uint8_t faults)
{
    // only react to changes
    if (faults == this->m_lastFaults) {
        return;
    }

    // fault cleared
    if ((this->m_lastFaults != 0) && (faults == 0)) {
        this->log_ACTIVITY_HI_ThermocoupleFaultCleared();
    }

    // open circuit
    if (faults & 0x1) {
        this->log_ACTIVITY_HI_ThermocoupleOpen();
    }

    // short to ground
    if (faults & 0x2) {
        this->log_ACTIVITY_HI_ThermocoupleShortToGnd();
    }

    // short to VCC
    if (faults & 0x4) {
        this->log_ACTIVITY_HI_ThermocoupleShortToVcc();
    }

    this->m_lastFaults = faults;
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void MAX31855Manager ::run_handler(FwIndexType portNum, U32 context) {
    uint8_t tx[4] = {0,0,0,0};
    uint8_t rx[4] = {0};

    Fw::Buffer writeBuffer(tx, sizeof(tx));
    Fw::Buffer readBuffer(rx, sizeof(rx));

    this->spiReadWrite_out(
        0,
        writeBuffer,
        readBuffer
    );

    uint32_t raw =
        (rx[0] << 24) |
        (rx[1] << 16) |
        (rx[2] << 8)  |
         rx[3];

    float temp = decodeThermocouple(raw);
    float coldJunctionTemp = decodeColdJunction(raw);
    uint8_t faults = decodeFaults(raw);

    if (faults == 0) {
            this->tlmWrite_THERMOCOUPLE_TEMP(temp);
            this->tlmWrite_COLD_JUNCTION_TEMP(coldJunctionTemp);
        }

    this->tlmWrite_THERMOCOUPLE_FAULT(faults);
}

}  // namespace Mara
