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
    
    //! method for reacting on parameter updates
    //! We're going to use that for reconfiguring the ltr
    void parameterUpdated(FwPrmIdType id  //!< The parameter ID
                          ) override;

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

    //! Implementation for action doReset of state machine Mara_LtrStateMachine
    //!
    //! Perform reset commands
    void Mara_LtrStateMachine_action_doReset(SmId smId,                                 //!< The state machine id
                                                   Mara_LtrStateMachine::Signal signal  //!< The signal
                                                   ) override;

    //! Implementation for action checkReset of state machine Mara_LtrStateMachine
    //!
    //! Check reset took place
    void Mara_LtrStateMachine_action_checkReset(SmId smId,  //!< The state machine id
                                                      Mara_LtrStateMachine::Signal signal  //!< The signal
                                                      ) override;

    //! Implementation for action doConfigure of state machine Mara_LtrStateMachine
    //!
    //! Perform configure commands
    void Mara_LtrStateMachine_action_doConfigure(SmId smId,  //!< The state machine id
                                                       Mara_LtrStateMachine::Signal signal  //!< The signal
                                                       ) override;

    //! Implementation for action doRead of state machine Mara_LtrStateMachine
    //!
    //! Read the sensor
    void Mara_LtrStateMachine_action_doRead(SmId smId,                                 //!< The state machine id
                                                  Mara_LtrStateMachine::Signal signal  //!< The signal
                                                  ) override;

    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Reset the Ltr
    Drv::I2cStatus reset();

    //! Read the ALS_CONTR register value
    Drv::I2cStatus read_ALS_CONTR(U8& value);

    //! Configure the Ltr
    //! This will both configure the gain and enable SW
    //! Done this way cause there's only one register controlling
    //! that behaviour
    Drv::I2cStatus configure_device();

    //! Read Ltr data
    //! I don't know if that's important - we don't actually check whether the device produced new data
    //! I think it may be irrelevant
    Drv::I2cStatus read(LtrData& LtrData);

    //! Write to the Ltr bus and handle errors
    Drv::I2cStatus bus_write(Fw::Buffer& writeBuffer, Fw::Buffer& readBuffer);

    //! Deserializes raw data from the bus
    Ltr::RawLtrData deserialize_raw_data(Fw::Buffer& buffer);

    //! Convert raw data into more meaningfull result for telemetry.
    LtrData convert_raw_data(Ltr::RawLtrData& rawData, const LtrGain& gain);

    //! Acceleration range to register value
    static U8 gain_to_register(LtrGain gain);

  private:
    U8 m_address;
};

}  // namespace Mara

#endif
