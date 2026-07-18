// ======================================================================
// \title  GPIOWatcher.hpp
// \author fsowa
// \brief  hpp file for GPIOWatcher component implementation class
// ======================================================================

#ifndef Mara_GPIOWatcher_HPP
#define Mara_GPIOWatcher_HPP

#include "Mara/Components/GPIOWatcher/GPIOWatcherComponentAc.hpp"
#include <atomic>

namespace Mara {

class GPIOWatcher final : public GPIOWatcherComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct GPIOWatcher object
    GPIOWatcher(const char* const compName  //!< The component name
    );

    //! Destroy GPIOWatcher object
    ~GPIOWatcher();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------
    //! Handler implementation for schedIn
    //!
    //! Preferably this would be connected to a rate group fast enough
    //! To do debouncing quick
    //! It's not a requirement though
    //! Since if gpio signals are late we don't really care
    void schedIn_handler(FwIndexType portNum,  //!< The port number
                         U32 context           //!< The call order
                         ) override;
    
    static constexpr int m_consecutive_highs_threshold{4};
    int m_LO_counter{0};
    bool m_LO_signal_emitted{false};
    int m_SOE_counter{0};
    bool m_SOE_signal_emitted{false};
    int m_EODS_counter{0};
    bool m_EODS_signal_emitted{false};
};

}  // namespace Mara

#endif
