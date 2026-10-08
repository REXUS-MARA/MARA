// ======================================================================
// \title  Bmp280Manager.hpp
// \author fsowa
// \brief  hpp file for Bmp280Manager component implementation class
// ======================================================================

#ifndef Mara_Bmp280Manager_HPP
#define Mara_Bmp280Manager_HPP

#include "Mara/Components/Bmp280Manager/Bmp280ManagerComponentAc.hpp"
#include "Mara/Components/Bmp280Manager/bmp2/bmp2.h"

namespace Mara {

class Bmp280Manager final : public Bmp280ManagerComponentBase {
  public:
    //! Readings per data product container (20 Hz -> one container every 25 s).
    //! The container must fit in one DataProducts buffer (dpBufferStoreSize in the deployment config).
    static constexpr FwSizeType RECORD_COUNT = 500;

    //! Data size of one container: the calibration and configuration records followed by RECORD_COUNT readings
    static constexpr FwSizeType CONTAINER_DATA_SIZE = (Bmp280Calib::SERIALIZED_SIZE + sizeof(FwDpIdType)) +
                                                      (Bmp280Config::SERIALIZED_SIZE + sizeof(FwDpIdType)) +
                                                      RECORD_COUNT * (Bmp280DataTimed::SERIALIZED_SIZE + sizeof(FwDpIdType));

    //! Size of one DataProducts buffer, mirrors DataProductsConfig::BuffMgr::dpBufferStoreSize
    static constexpr FwSizeType DP_BUFFER_SIZE = 20000;

    static_assert(Fw::DpContainer::getPacketSizeForDataSize(CONTAINER_DATA_SIZE) <= DP_BUFFER_SIZE,
                  "BMP280 data product container does not fit in a DataProducts buffer");

    //! Largest I2C write the Bosch API issues (register + interleaved register/value pairs)
    static constexpr FwSizeType MAX_WRITE_SIZE = 8;

    //! Number of bytes in the pressure + temperature data registers (0xF7..0xFC)
    static constexpr FwSizeType DATA_REGISTERS_SIZE = 6;

    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct Bmp280Manager object
    Bmp280Manager(const char* const compName  //!< The component name
    );

    //! Destroy Bmp280Manager object
    ~Bmp280Manager();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for run
    //!
    //! Rate group tick: advances the sensor state machine
    void run_handler(FwIndexType portNum,  //!< The port number
                     U32 context           //!< The call order
                     ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command RESET
    //!
    //! Force a sensor reset
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

  private:
    // ----------------------------------------------------------------------
    // Parameter updates
    // ----------------------------------------------------------------------

    //! Reconfigure (or fully reset, for the address) when a parameter changes
    void parameterUpdated(FwPrmIdType id) override;

  private:
    // ----------------------------------------------------------------------
    // Bosch API glue (Bmp280Helpers.cpp)
    // ----------------------------------------------------------------------

    //! Bosch read callback: reads `length` bytes starting at `regAddr`
    static BMP2_INTF_RET_TYPE i2cRead(uint8_t regAddr, uint8_t* regData, uint32_t length, const void* intfPtr);

    //! Bosch write callback: writes `regAddr` followed by `length` bytes (interleaved for burst writes)
    static BMP2_INTF_RET_TYPE i2cWrite(uint8_t regAddr, const uint8_t* regData, uint32_t length, const void* intfPtr);

    //! Bosch delay callback
    static void delayUs(uint32_t period, void* intfPtr);

    //! Logs a failed Bosch call with the most specific event available
    void reportBoschFailure(int8_t result);

    //! Sends `success` when the Bosch call succeeded, otherwise reports it and sends `error`
    void signalResult(int8_t result);

    //! Reads the configuration parameters
    Bmp280Config configFromParams();

    //! Converts the configuration to the Bosch structure
    static bmp2_config toBosch(const Bmp280Config& config);

    //! Reads the data registers and compensates them; `status` holds the Bosch result
    int8_t readSensor(bmp2_uncomp_data& raw, bmp2_data& compensated);

    //! Calibration values as an FPP struct, for the data product
    Bmp280Calib calibration() const;

    //! Adds a reading to the data product container, sending it when full
    void recordReading(const Bmp280Data& data, const bmp2_uncomp_data& raw, I8 status);

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    bmp2_dev m_dev;                    //!< Bosch device handle (holds the calibration)
    U8 m_address;                      //!< Current I2C address
    Drv::I2cStatus m_lastI2cStatus;    //!< Status of the last I2C transfer, for events
    Bmp280Config m_config;             //!< Configuration the sensor is running with
    DpContainer m_container;           //!< Data product container (currently allocated)
    bool m_containerValid;             //!< Whether m_container is allocated
    FwSizeType m_count;                //!< Readings serialized into m_container
};

}  // namespace Mara

#endif
