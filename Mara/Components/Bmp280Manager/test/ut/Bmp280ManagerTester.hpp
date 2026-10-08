// ======================================================================
// \title  Bmp280ManagerTester.hpp
// \author fsowa
// \brief  hpp file for Bmp280Manager component test harness implementation class
// ======================================================================

#ifndef Mara_Bmp280ManagerTester_HPP
#define Mara_Bmp280ManagerTester_HPP

#include "Mara/Components/Bmp280Manager/Bmp280Manager.hpp"
#include "Mara/Components/Bmp280Manager/Bmp280ManagerGTestBase.hpp"

#include <array>
#include <vector>

namespace Mara {

class Bmp280ManagerTester final : public Bmp280ManagerGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Room for one full data product of telemetry plus events
    static const FwSizeType MAX_HISTORY_SIZE = 1200;

    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

    // Ticks from power-on until the first reading (one SM state per tick)
    static const U32 TICKS_TO_FIRST_READ = 5;

    using State = Bmp280ManagerComponentBase::Mara_I2CSensorStateMachine::State;

    //! A decoded data product container
    struct Decoded {
        Bmp280Calib calib;
        Bmp280Config config;
        std::vector<Bmp280DataTimed> readings;
    };

  public:
    Bmp280ManagerTester();

    ~Bmp280ManagerTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    void testStartupSequence();

    void testWaitsForNvmCopy();

    void testWrongChipId();

    void testDatasheetCompensation();

    void testConfigRegisters();

    void testReconfigureOnParameter();

    void testBusErrorRecovery();

    void testLowPressureClamp();

    void testDataProduct();

    void testReconfigureClosesContainer();

    void testResetCommand();

  private:
    // ----------------------------------------------------------------------
    // Fake BMP280 on the I2C bus
    // ----------------------------------------------------------------------

    Drv::I2cStatus from_busWrite_handler(FwIndexType portNum, U32 addr, Fw::Buffer& serBuffer) override;

    Drv::I2cStatus from_busWriteRead_handler(FwIndexType portNum,
                                             U32 addr,
                                             Fw::Buffer& writeBuffer,
                                             Fw::Buffer& readBuffer) override;

    Fw::Success::T productGet_handler(FwDpIdType id, FwSizeType size, Fw::Buffer& buffer) override;

    void productSend_handler(FwDpIdType id, const Fw::Buffer& buffer) override;

  private:
    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    void connectPorts();

    void initComponents();

    void tick(U32 count = 1);

    //! Ticks until the first container is sent; returns it decoded
    Decoded fillContainer();

    //! Decodes a container sent by the component
    Decoded decodeSent(FwSizeType index);

    State state() const;

    void setAdc(U32 adcP, U32 adcT);

    //! Decodes the data part of a container (calibration and configuration records + readings)
    Decoded decode(const U8* data, FwSizeType size) const;

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    Bmp280Manager component;

    //! Register map of the fake sensor
    std::array<U8, 256> m_regs{};

    //! Address the fake sensor answers on
    U32 m_deviceAddress = 0x77;

    //! When true every transfer fails
    bool m_busDown = false;

    //! Soft resets received
    U32 m_resets = 0;

    //! Storage for data product buffers handed to the component
    std::vector<std::vector<U8>> m_dpStorage;

    //! Copies of the containers sent by the component
    std::vector<std::vector<U8>> m_sentProducts;
};

}  // namespace Mara

#endif
