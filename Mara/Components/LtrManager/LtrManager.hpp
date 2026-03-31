// ======================================================================
// \title  LtrManager.hpp
// \author fsowa
// \brief  hpp file for LtrManager component implementation class
// ======================================================================

#ifndef Mara_LtrManager_HPP
#define Mara_LtrManager_HPP

#include "Mara/Components/LtrManager/LtrManagerComponentAc.hpp"
#include "Mara/Components/LtrManager/LtrTypes.hpp"

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

    //! Configure the device address
    void configure(U8 device_address = Ltr::DEVICE_DEFAULT_ADDRESS);

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
    // void Mara_I2CSensorStateMachine_action_doConfigure(SmId smId,  //!< The state machine id
    //                                                    Mara_I2CSensorStateMachine::Signal signal  //!< The signal
    //                                                    ) override;

    //! Implementation for action doRead of state machine Mara_I2CSensorStateMachine
    //!
    //! Read the sensor
    void Mara_I2CSensorStateMachine_action_doRead(SmId smId,                                 //!< The state machine id
                                                  Mara_I2CSensorStateMachine::Signal signal  //!< The signal
                                                  ) override;

    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Reset the Ltr
    Drv::I2cStatus reset();

    //! Read the ALS_CONTR register value
    Drv::I2cStatus read_ALS_CONTR(U8& value);

    //! Enable on the Ltr
    Drv::I2cStatus enable();

    //! Configure the Ltr
    // Drv::I2cStatus configure_device();

    //! Read Ltr data
    Drv::I2cStatus read(LtrData& LtrData);

    //! Write to the Ltr bus and handle errors
    Drv::I2cStatus bus_write(Fw::Buffer& writeBuffer, Fw::Buffer& readBuffer);

    //! Deserializes raw data from the bus
    Ltr::RawLtrData deserialize_raw_data(Fw::Buffer& buffer);

    //! Convert raw data into more meaningfull result for telemetry.
    LtrData convert_raw_data(Ltr::RawLtrData& rawData);

  private:
    U8 m_address;
};

}  // namespace Mara

#endif
