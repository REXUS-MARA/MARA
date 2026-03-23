// ======================================================================
// \title  LtrManager.hpp
// \author fsowa
// \brief  hpp file for LtrManager component implementation class
// ======================================================================

#ifndef Mara_LtrManager_HPP
#define Mara_LtrManager_HPP

#include "Mara/Components/LtrManager/LtrManagerComponentAc.hpp"

namespace Mara {

class LtrManager final : public LtrManagerComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct LtrManager object
    LtrManager(const char* const compName  //!< The component name
    );

    //! Destroy LtrManager object
    ~LtrManager();

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
