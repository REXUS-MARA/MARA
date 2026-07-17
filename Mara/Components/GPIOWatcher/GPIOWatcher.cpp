// ======================================================================
// \title  GPIOWatcher.cpp
// \author fsowa
// \brief  cpp file for GPIOWatcher component implementation class
// ======================================================================

#include "Mara/Components/GPIOWatcher/GPIOWatcher.hpp"

namespace Mara {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

GPIOWatcher ::GPIOWatcher(const char* const compName) : GPIOWatcherComponentBase(compName) {}

GPIOWatcher ::~GPIOWatcher() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void GPIOWatcher ::EODSPinInterrupt_handler(FwIndexType portNum, U32 context) {
    m_EODS_primed = true;
}

void GPIOWatcher ::LOPinInterrupt_handler(FwIndexType portNum, U32 context) {
    m_LO_primed = true;
}

void GPIOWatcher ::SOEPinInterrupt_handler(FwIndexType portNum, U32 context) {
    m_SOE_primed = true;
}

void GPIOWatcher ::schedIn_handler(FwIndexType portNum, U32 context) {
    if(m_EODS_primed) {
        // FW_ASSERT(isConnected_EODSPinRead_OutputPort(0));
        Fw::Logic pin_state{Fw::Logic::LOW};
        EODSPinRead_out(0, pin_state);
        if(pin_state == Fw::Logic::HIGH){
            m_EODS_counter++;
        } else {
            // so here we detected that the line was primed
            // but it went down
            // so, it was a spike
            log_ACTIVITY_HI_EODSSpike();
            m_EODS_primed = false;
            m_EODS_counter = 0;
        }
        if(m_EODS_counter >= m_rising_edges_threshold){
            log_ACTIVITY_HI_EODSDetected();
            EODSHigh_out(0);
            m_EODS_primed = false;
            m_EODS_counter = 0;
        }
    }

    if(m_LO_primed) {
        // FW_ASSERT(isConnected_LOPinRead_OutputPort(0));
        Fw::Logic pin_state{Fw::Logic::LOW};
        LOPinRead_out(0, pin_state);
        if(pin_state == Fw::Logic::HIGH){
            m_LO_counter++;
        } else {
            log_ACTIVITY_HI_LOSpike();
            m_LO_primed = false;
            m_LO_counter = 0;
        }
        if(m_LO_counter >= m_rising_edges_threshold){
            log_ACTIVITY_HI_LODetected();
            LOHigh_out(0);
            m_LO_primed = false;
            m_LO_counter = 0;
        }
    }

    if(m_SOE_primed) {
        // FW_ASSERT(isConnected_SOEPinRead_OutputPort(0));
        Fw::Logic pin_state{Fw::Logic::LOW};
        SOEPinRead_out(0, pin_state);
        if(pin_state == Fw::Logic::HIGH){
            m_SOE_counter++;
        } else {
            log_ACTIVITY_HI_SOESpike();
            m_SOE_primed = false;
            m_SOE_counter = 0;
        }
        if(m_SOE_counter >= m_rising_edges_threshold){
            log_ACTIVITY_HI_SOEDetected();
            SOEHigh_out(0);
            m_SOE_primed = false;
            m_SOE_counter = 0;
        }
    }
}

}  // namespace Mara
