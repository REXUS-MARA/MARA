// ======================================================================
// \title  MAX31855Manager.hpp
// \author john
// \brief  hpp file for MAX31855Manager component implementation class
// ======================================================================

#ifndef Mara_MAX31855Manager_HPP
#define Mara_MAX31855Manager_HPP

#include "Mara/Components/MAX31855Manager/MAX31855ManagerComponentAc.hpp"

namespace Mara {

class MAX31855Manager final : public MAX31855ManagerComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct MAX31855Manager object
    MAX31855Manager(const char* const compName  //!< The component name
    );

    //! Destroy MAX31855Manager object
    ~MAX31855Manager();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------
    
    uint8_t m_lastFaults = 0;

    void handleFaults(uint8_t faults);

    //! Handler implementation for run
    void run_handler(
          FwIndexType portNum, //!< The port number
          U32 context //!< The call order
      ) = 0;
    
    
};

}  // namespace Mara

#endif
