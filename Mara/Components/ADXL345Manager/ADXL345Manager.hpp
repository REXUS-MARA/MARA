// ======================================================================
// \title  ADXL345Manager.hpp
// \author imanol
// \brief  hpp file for ADXL345Manager component implementation class
// ======================================================================

#ifndef Mara_ADXL345Manager_HPP
#define Mara_ADXL345Manager_HPP

#include "Mara/Components/ADXL345Manager/ADXL345ManagerComponentAc.hpp"

namespace Mara {

class ADXL345Manager final : public ADXL345ManagerComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct ADXL345Manager object
    ADXL345Manager(const char* const compName  //!< The component name
    );

    //! Destroy ADXL345Manager object
    ~ADXL345Manager();
};

}  // namespace Mara

#endif
