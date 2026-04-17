// ======================================================================
// \title  INA228.hpp
// \author sebahs
// \brief  hpp file for INA228 component implementation class
// ======================================================================

#ifndef Mara_INA228_HPP
#define Mara_INA228_HPP

#include "Mara/Components/INA228/INA228ComponentAc.hpp"

namespace Mara {

class INA228 final : public INA228ComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct INA228 object
    INA228(const char* const compName  //!< The component name
    );

    //! Destroy INA228 object
    ~INA228();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command TODO
    //!
    //! TODO
    void TODO_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                         U32 cmdSeq            //!< The command sequence number
                         ) override;
};

}  // namespace Mara

#endif
