// ======================================================================
// \title  GPIOWatcher.cpp
// \author fsowa
// \brief  cpp file for GPIOWatcher component implementation class
// ======================================================================

#include "Mara/Components/GPIOWatcher/GPIOWatcher.hpp"
#include <functional>

namespace Mara {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

GPIOWatcher ::GPIOWatcher(const char* const compName) : GPIOWatcherComponentBase(compName) {}

GPIOWatcher ::~GPIOWatcher() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void GPIOWatcher ::schedIn_handler(FwIndexType portNum, U32 context) {
    auto check_pins = [&](   std::function<bool(FwIndexType)> prerequisite,
                            std::function<Drv::GpioStatus(FwIndexType, Fw::Logic&)> pin_read,
                            std::function<void()> log_spike,
                            std::function<void()> log_signal,
                            std::function<void(FwIndexType)> emit_signal,
                            int & state_counter,
                            bool & signal_emmited){
        FW_ASSERT(prerequisite(0));
        Fw::Logic pin_state{Fw::Logic::LOW};
        pin_read(0, pin_state);
        if(pin_state == Fw::Logic::HIGH and not signal_emmited){
            state_counter++;
            if(state_counter >= m_consecutive_highs_threshold){
                log_signal();
                emit_signal(0);
                state_counter = 0;
                signal_emmited = true;
            }
        } else if (pin_state == Fw::Logic::LOW ) {
            if(state_counter > 0 and state_counter < m_consecutive_highs_threshold){
                log_spike();
            }
            state_counter = 0;
            signal_emmited = false;
        }

    };
    // I'm really sorry for this monstrosity below
    // KISS was violated here
    // I promise that no LLM was used here, but maybe it should have
    check_pins( [this]<typename ...Ts>(Ts&& ...ts) {return isConnected_EODSPinRead_OutputPort(std::forward<Ts>(ts)...);},
                [this]<typename ...Ts>(Ts&& ...ts) {return EODSPinRead_out(std::forward<Ts>(ts)...);},
                [this]<typename ...Ts>(Ts&& ...ts) {return log_ACTIVITY_HI_EODSSpike(std::forward<Ts>(ts)...);},
                [this]<typename ...Ts>(Ts&& ...ts) {return log_ACTIVITY_HI_EODSDetected(std::forward<Ts>(ts)...);},
                [this]<typename ...Ts>(Ts&& ...ts) {return EODSHigh_out(std::forward<Ts>(ts)...);},
                m_EODS_counter,
                m_EODS_signal_emitted );

    check_pins( [this]<typename ...Ts>(Ts&& ...ts) {return isConnected_LOPinRead_OutputPort(std::forward<Ts>(ts)...);},
                [this]<typename ...Ts>(Ts&& ...ts) {return LOPinRead_out(std::forward<Ts>(ts)...);},
                [this]<typename ...Ts>(Ts&& ...ts) {return log_ACTIVITY_HI_LOSpike(std::forward<Ts>(ts)...);},
                [this]<typename ...Ts>(Ts&& ...ts) {return log_ACTIVITY_HI_LODetected(std::forward<Ts>(ts)...);},
                [this]<typename ...Ts>(Ts&& ...ts) {return LOHigh_out(std::forward<Ts>(ts)...);},
                m_LO_counter,
                m_LO_signal_emitted);

    check_pins( [this]<typename ...Ts>(Ts&& ...ts) {return isConnected_SOEPinRead_OutputPort(std::forward<Ts>(ts)...);},
                [this]<typename ...Ts>(Ts&& ...ts) {return SOEPinRead_out(std::forward<Ts>(ts)...);},
                [this]<typename ...Ts>(Ts&& ...ts) {return log_ACTIVITY_HI_SOESpike(std::forward<Ts>(ts)...);},
                [this]<typename ...Ts>(Ts&& ...ts) {return log_ACTIVITY_HI_SOEDetected(std::forward<Ts>(ts)...);},
                [this]<typename ...Ts>(Ts&& ...ts) {return SOEHigh_out(std::forward<Ts>(ts)...);},
                m_SOE_counter,
                m_SOE_signal_emitted);
}

}  // namespace Mara
