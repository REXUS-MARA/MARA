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
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for run
    //!
    //! Scheduling port for reading from IMU and writing to telemetry
    //! Stolen from the Mpu, maybe not needed? Idk
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

    //! Implementation for action doReset of state machine Mara_I2CSensorStateMachine
    //!
    //! Perform reset commands
    void Mara_I2CSensorStateMachine_action_doReset(SmId smId,                                 //!< The state machine id
                                                   Mara_I2CSensorStateMachine::Signal signal  //!< The signal
                                                   ) override;

    //! Implementation for action checkReset of state machine Mara_I2CSensorStateMachine
    //!
    //! Check reset took place
    void Mara_I2CSensorStateMachine_action_checkReset(SmId smId,  //!< The state machine id
                                                      Mara_I2CSensorStateMachine::Signal signal  //!< The signal
                                                      ) override;

    //! Implementation for action doEnable of state machine Mara_I2CSensorStateMachine
    //!
    //! Perform enable commands
    void Mara_I2CSensorStateMachine_action_doEnable(SmId smId,                                 //!< The state machine id
                                                    Mara_I2CSensorStateMachine::Signal signal  //!< The signal
                                                    ) override;

    //! Implementation for action doConfigure of state machine Mara_I2CSensorStateMachine
    //!
    //! Perform configure commands
    void Mara_I2CSensorStateMachine_action_doConfigure(SmId smId,  //!< The state machine id
                                                       Mara_I2CSensorStateMachine::Signal signal  //!< The signal
                                                       ) override;

    //! Implementation for action doRead of state machine Mara_I2CSensorStateMachine
    //!
    //! Read the sensor
    void Mara_I2CSensorStateMachine_action_doRead(SmId smId,                                 //!< The state machine id
                                                  Mara_I2CSensorStateMachine::Signal signal  //!< The signal
                                                  ) override;
};

}  // namespace Mara

#endif
