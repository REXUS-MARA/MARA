// ======================================================================
// \title  FatalHandler.hpp
// \author fsowa
// \brief  hpp file for FatalHandler component implementation class
// ======================================================================

#ifndef Mara_FatalHandler_HPP
#define Mara_FatalHandler_HPP

#include "Mara/Components/FatalHandler/FatalHandlerComponentAc.hpp"

#include <atomic>

namespace Mara {

class FatalHandler final : public FatalHandlerComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct FatalHandler object
    FatalHandler(const char* const compName  //!< The component name
    );

    //! Destroy FatalHandler object
    ~FatalHandler();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for FatalReceive
    //!
    //! FATAL announcements from the event manager. Called on the thread that logged the FATAL.
    void FatalReceive_handler(FwIndexType portNum,  //!< The port number
                              FwEventIdType Id      //!< The ID of the FATAL event
                              ) override;

  private:
    std::atomic<U32> m_fatalCount{0};  //!< FATALs since boot
};

}  // namespace Mara

#endif
