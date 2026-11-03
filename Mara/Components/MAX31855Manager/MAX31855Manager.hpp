// ======================================================================
// \title  MAX31855Manager.hpp
// \author john
// \brief  hpp file for MAX31855Manager component implementation class
// ======================================================================

#ifndef Mara_MAX31855Manager_HPP
#define Mara_MAX31855Manager_HPP

#include "Mara/Components/MAX31855Manager/MAX31855ManagerComponentAc.hpp"
#include "Mara/Components/MAX31855Manager/ThermoDataTimedSerializableAc.hpp"

namespace Mara {

class MAX31855Manager final : public MAX31855ManagerComponentBase {

  public:
    MAX31855Manager(const char* const compName);
    ~MAX31855Manager();

  private:
    static constexpr FwSizeType RECORD_COUNT = 100;

    void run_handler(FwIndexType portNum, U32 context) override;

    //! Clocks one 32-bit frame out of the MAX31855
    Drv::SpiStatus readFrame(U32& frame);

    //! Decodes a raw frame into engineering units
    static ThermoData decodeFrame(U32 frame);

    //! True if the reserved bits are 0 per data sheet
    static bool frameValid(U32 frame);

    FwSizeType m_count;
    DpContainer m_container;
    bool m_containerValid;
    bool m_initialized;
    U8 m_lastFaults;  //!< Fault bits of the previous reading, to log only on change
};

}  // namespace Mara
#endif
