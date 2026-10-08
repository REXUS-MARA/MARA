// ======================================================================
// \title  INA228.hpp
// \author sebahs
// \brief  hpp file for INA228 component implementation class
// ======================================================================

#ifndef Mara_INA228_HPP
#define Mara_INA228_HPP

#include "Mara/Components/INA228/INA228ComponentAc.hpp"
#include "Mara/Components/INA228/INATypes.hpp"

namespace Mara {

class INA228 final : public INA228ComponentBase {
  public:
    // Container size
    constexpr static const FwSizeType RECORD_COUNT = 600;  //!< Number of records of each type in the data product
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
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for run
    //!
    //! Sync port, grandfathered in from LTR-303
    void run_handler(FwIndexType portNum,  //!< The port number
                     U32 context           //!< The call order
                     ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command RESET
    //!
    //! Command to force a RESET
    void RESET_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                          U32 cmdSeq            //!< The command sequence number
                          ) override;

  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine actions
    // ----------------------------------------------------------------------

    //! Implementation for action doReset of state machine Mara_INAStateMachine
    //!
    //! Perform reset commands
    void Mara_INAStateMachine_action_doReset(SmId smId,                                 //!< The state machine id
                                                   Mara_INAStateMachine::Signal signal  //!< The signal
                                                   ) override;

    //! Implementation for action checkReset of state machine Mara_INAStateMachine
    //!
    //! Check reset took place
    void Mara_INAStateMachine_action_checkReset(SmId smId,  //!< The state machine id
                                                      Mara_INAStateMachine::Signal signal  //!< The signal
                                                      ) override;

    //! Implementation for action doEnable of state machine Mara_INAStateMachine
    //!
    //! Perform enable commands
//    void Mara_INAStateMachine_action_doEnable(SmId smId,                                 //!< The state machine id
  //                                                  Mara_INAStateMachine::Signal signal  //!< The signal
    //                                                ) override;

    //! Implementation for action doConfigure of state machine Mara_INAStateMachine
    //!
    //! Perform configure commands
    void Mara_INAStateMachine_action_doConfigure(SmId smId,  //!< The state machine id
                                                       Mara_INAStateMachine::Signal signal  //!< The signal
                                                       ) override;

    //! Implementation for action doRead of state machine Mara_INAStateMachine
    //!
    //! Read the sensor
    void Mara_INAStateMachine_action_doRead(SmId smId,                                 //!< The state machine id
                                                  Mara_INAStateMachine::Signal signal  //!< The signal

                                                  ) override;

    // Private funcs

    // Private vars
    U8 m_address;
    FwSizeType m_count;       //!< Count of serialized records
    DpContainer m_container;  //!< Data product container (currently allocated)
    bool m_containerValid;    //!< Whether the container is valid

 public:
    Drv::I2cStatus reset();
    Drv::I2cStatus check_reset();
    Drv::I2cStatus configure_device();
    Drv::I2cStatus read_register(U8 registerAddress, I32* value);
    Drv::I2cStatus write_register(U8 registerAddress, U16 value);
    Drv::I2cStatus read(INAData* data);

};
}  // namespace Mara

#endif
