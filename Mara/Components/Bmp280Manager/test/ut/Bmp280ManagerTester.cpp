// ======================================================================
// \title  Bmp280ManagerTester.cpp
// \author fsowa
// \brief  cpp file for Bmp280Manager component test harness implementation class
// ======================================================================

#include "Bmp280ManagerTester.hpp"

#include <algorithm>

namespace Mara {

namespace {
// Registers (datasheet section 4.3)
constexpr U8 REG_CALIB = 0x88;
constexpr U8 REG_CHIP_ID = 0xD0;
constexpr U8 REG_RESET = 0xE0;
constexpr U8 REG_STATUS = 0xF3;
constexpr U8 REG_CTRL_MEAS = 0xF4;
constexpr U8 REG_CONFIG = 0xF5;
constexpr U8 REG_PRESS_MSB = 0xF7;
constexpr U8 RESET_COMMAND = 0xB6;

// Local copies: gtest takes its arguments by reference, which needs a definition in C++14
constexpr FwSizeType RECORD_COUNT = Bmp280Manager::RECORD_COUNT;
constexpr FwSizeType CONTAINER_DATA_SIZE = Bmp280Manager::CONTAINER_DATA_SIZE;

// Datasheet section 3.12 example ADC values
constexpr U32 EXAMPLE_ADC_P = 415148;
constexpr U32 EXAMPLE_ADC_T = 519888;

// Datasheet section 3.12 example trimming parameters
constexpr U16 DIG_T1 = 27504;
constexpr I16 DIG_T2 = 26435;
constexpr I16 DIG_T3 = -1000;
constexpr U16 DIG_P1 = 36477;
constexpr std::array<I16, 8> DIG_P2_P9{{-10685, 3024, 2855, 140, -7, 15500, -14600, 6000}};

void putLe16(std::array<U8, 256>& regs, U8 reg, U16 value) {
    regs[reg] = static_cast<U8>(value & 0xFF);
    regs[reg + 1] = static_cast<U8>(value >> 8);
}
}  // namespace

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

Bmp280ManagerTester ::Bmp280ManagerTester()
    : Bmp280ManagerGTestBase("Bmp280ManagerTester", Bmp280ManagerTester::MAX_HISTORY_SIZE), component("Bmp280Manager") {
    this->initComponents();
    this->connectPorts();

    this->m_regs[REG_CHIP_ID] = 0x58;
    putLe16(this->m_regs, REG_CALIB, DIG_T1);
    putLe16(this->m_regs, REG_CALIB + 2, static_cast<U16>(DIG_T2));
    putLe16(this->m_regs, REG_CALIB + 4, static_cast<U16>(DIG_T3));
    putLe16(this->m_regs, REG_CALIB + 6, DIG_P1);
    for (FwSizeType i = 0; i < DIG_P2_P9.size(); i++) {
        putLe16(this->m_regs, static_cast<U8>(REG_CALIB + 8 + 2 * i), static_cast<U16>(DIG_P2_P9[i]));
    }
    this->setAdc(EXAMPLE_ADC_P, EXAMPLE_ADC_T);

    this->component.loadParameters();
}

Bmp280ManagerTester ::~Bmp280ManagerTester() {
    // The queued component owns a message queue
    static_cast<Bmp280ManagerComponentBase&>(this->component).deinit();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void Bmp280ManagerTester ::testStartupSequence() {
    ASSERT_EQ(this->state(), State::RESET);
    this->tick();
    EXPECT_EQ(this->m_resets, 1u);
    this->tick();
    ASSERT_EQ(this->state(), State::WAIT_RESET);
    this->tick();
    ASSERT_EQ(this->state(), State::ENABLE);
    this->tick();
    ASSERT_EQ(this->state(), State::CONFIGURE);
    ASSERT_TLM_Reading_SIZE(0);
    this->tick();
    ASSERT_EQ(this->state(), State::RUN);
    ASSERT_TLM_Reading_SIZE(1);
    ASSERT_EVENTS_Configured_SIZE(1);
    // Standard resolution (11.5 ms) + 0.5 ms standby
    ASSERT_EVENTS_Configured(0, 12000);
    ASSERT_EVENTS_I2cError_SIZE(0);

    this->tick(3);
    ASSERT_TLM_Reading_SIZE(4);
}

void Bmp280ManagerTester ::testWaitsForNvmCopy() {
    this->m_regs[REG_STATUS] = 0x01;  // im_update: NVM copy still running
    this->tick(10);
    ASSERT_EQ(this->state(), State::WAIT_RESET);
    ASSERT_EVENTS_SIZE(0);

    this->m_regs[REG_STATUS] = 0x00;
    this->tick(4);
    ASSERT_EQ(this->state(), State::RUN);
}

void Bmp280ManagerTester ::testWrongChipId() {
    this->m_regs[REG_CHIP_ID] = 0x60;  // a BME280
    this->tick(3);
    ASSERT_EQ(this->state(), State::ENABLE);
    ASSERT_EVENTS_ChipIdMismatch_SIZE(1);
    ASSERT_EVENTS_ChipIdMismatch(0, 0x60);
    // The error signal is handled on the next tick
    this->tick();
    ASSERT_EQ(this->state(), State::RESET);
    ASSERT_TLM_Reading_SIZE(0);
}

void Bmp280ManagerTester ::testDatasheetCompensation() {
    this->tick(TICKS_TO_FIRST_READ);
    ASSERT_TLM_Reading_SIZE(1);
    const Bmp280Data& reading = this->tlmHistory_Reading->at(0).arg;
    EXPECT_NEAR(reading.get_temperature(), 25.08f, 0.01f);
    EXPECT_NEAR(reading.get_pressure(), 100653.27f, 0.1f);
    ASSERT_TLM_Status_SIZE(1);
    ASSERT_TLM_Status(0, 0);
    ASSERT_EVENTS_OutOfSpec_SIZE(0);
}

void Bmp280ManagerTester ::testConfigRegisters() {
    this->tick(TICKS_TO_FIRST_READ);
    // Defaults: osrs_t = x1 (001), osrs_p = x4 (011), normal mode (11)
    EXPECT_EQ(this->m_regs[REG_CTRL_MEAS], 0x2F);
    // t_sb = 0.5 ms (000), filter off (000), SPI 3-wire off
    EXPECT_EQ(this->m_regs[REG_CONFIG], 0x00);
}

void Bmp280ManagerTester ::testReconfigureOnParameter() {
    this->tick(TICKS_TO_FIRST_READ);
    ASSERT_EQ(this->state(), State::RUN);

    this->paramSet_OVERSAMPLING(Bmp280OsMode::HIGH_RESOLUTION, Fw::ParamValid::VALID);
    this->paramSend_OVERSAMPLING(TEST_INSTANCE_ID, 0);
    this->paramSet_FILTER(Bmp280Filter::COEFF_4, Fw::ParamValid::VALID);
    this->paramSend_FILTER(TEST_INSTANCE_ID, 0);
    this->paramSet_STANDBY(Bmp280Standby::MS_62_5, Fw::ParamValid::VALID);
    this->paramSend_STANDBY(TEST_INSTANCE_ID, 0);

    this->tick(2);
    ASSERT_EQ(this->state(), State::RUN);
    // osrs_t = x1 (001), osrs_p = x8 (100), normal mode (11)
    EXPECT_EQ(this->m_regs[REG_CTRL_MEAS], 0x33);
    // t_sb = 62.5 ms (001), filter 4 (010)
    EXPECT_EQ(this->m_regs[REG_CONFIG], 0x28);
    ASSERT_EVENTS_Configured_SIZE(2);
}

void Bmp280ManagerTester ::testBusErrorRecovery() {
    this->tick(TICKS_TO_FIRST_READ);
    ASSERT_EQ(this->state(), State::RUN);

    this->m_busDown = true;
    this->tick();
    ASSERT_EVENTS_I2cError_SIZE(1);
    ASSERT_EVENTS_I2cError(0, 0x77, Drv::I2cStatus::I2C_READ_ERR);
    this->tick(10);
    ASSERT_EQ(this->state(), State::RESET);
    const FwSizeType readings = this->tlmHistory_Reading->size();

    this->m_busDown = false;
    this->tick(TICKS_TO_FIRST_READ);
    ASSERT_EQ(this->state(), State::RUN);
    ASSERT_EQ(this->tlmHistory_Reading->size(), readings + 1);
}

void Bmp280ManagerTester ::testLowPressureClamp() {
    // ~181 hPa with the example calibration: above ~9 km, below the sensor's 300 hPa range
    const U32 lowAdcP = 900000;
    this->setAdc(lowAdcP, EXAMPLE_ADC_T);
    this->tick(TICKS_TO_FIRST_READ);

    ASSERT_EQ(this->state(), State::RUN);
    ASSERT_TLM_Reading_SIZE(1);
    EXPECT_EQ(this->tlmHistory_Reading->at(0).arg.get_pressure(), 30000.0f);
    ASSERT_TLM_Status(0, BMP2_W_MIN_PRES);
    ASSERT_EVENTS_OutOfSpec_SIZE(1);
    ASSERT_EVENTS_OutOfSpec(0, BMP2_W_MIN_PRES);
    ASSERT_EVENTS_I2cError_SIZE(0);

    // The raw value needed to recompute the pressure is kept in the data product
    const Decoded decoded = this->fillContainer();
    ASSERT_EQ(decoded.readings.size(), RECORD_COUNT);
    EXPECT_EQ(decoded.readings[0].get_data().get_pressure(), 30000.0f);
    EXPECT_EQ(decoded.readings[0].get_rawPressure(), lowAdcP);
    EXPECT_EQ(decoded.readings[0].get_rawTemperature(), EXAMPLE_ADC_T);
    EXPECT_EQ(decoded.readings[0].get_status(), BMP2_W_MIN_PRES);
}

void Bmp280ManagerTester ::testDataProduct() {
    const Decoded decoded = this->fillContainer();
    ASSERT_PRODUCT_GET_SIZE(1);
    // The port carries the packet size: header + hashes + data
    ASSERT_PRODUCT_GET(0, this->component.getIdBase() + Bmp280ManagerComponentBase::ContainerId::ReadingContainer,
                       Fw::DpContainer::getPacketSizeForDataSize(CONTAINER_DATA_SIZE));
    EXPECT_EQ(decoded.calib.get_t1(), DIG_T1);
    EXPECT_EQ(decoded.calib.get_t2(), DIG_T2);
    EXPECT_EQ(decoded.calib.get_p9(), DIG_P2_P9[7]);
    EXPECT_EQ(decoded.config, Bmp280Config(Bmp280OsMode::STANDARD_RESOLUTION, Bmp280Filter::OFF, Bmp280Standby::MS_0_5));
    ASSERT_EQ(decoded.readings.size(), RECORD_COUNT);
    EXPECT_EQ(decoded.readings[0].get_rawPressure(), EXAMPLE_ADC_P);
    EXPECT_NEAR(decoded.readings.back().get_data().get_pressure(), 100653.27f, 0.1f);

    // The next reading starts a new container
    this->tick();
    ASSERT_PRODUCT_GET_SIZE(2);
}

void Bmp280ManagerTester ::testReconfigureClosesContainer() {
    this->tick(TICKS_TO_FIRST_READ + 9);
    ASSERT_PRODUCT_GET_SIZE(1);
    ASSERT_EQ(this->m_sentProducts.size(), 0u);

    // A bus error resets and reconfigures with the same settings: the container stays open
    this->m_busDown = true;
    this->tick();
    this->m_busDown = false;
    this->tick(TICKS_TO_FIRST_READ);
    ASSERT_EQ(this->state(), State::RUN);
    ASSERT_EQ(this->m_sentProducts.size(), 0u);

    // New settings: the open container (readings with the old settings) is sent early
    this->paramSet_FILTER(Bmp280Filter::COEFF_16, Fw::ParamValid::VALID);
    this->paramSend_FILTER(TEST_INSTANCE_ID, 0);
    this->tick();
    ASSERT_EQ(this->m_sentProducts.size(), 1u);
    const Decoded first = this->decodeSent(0);
    EXPECT_EQ(first.config.get_filter(), Bmp280Filter::OFF);
    EXPECT_EQ(first.readings.size(), 11u);

    // The next container starts with the new configuration
    this->tick(RECORD_COUNT + 1);
    ASSERT_EQ(this->m_sentProducts.size(), 2u);
    const Decoded second = this->decodeSent(1);
    EXPECT_EQ(second.config.get_filter(), Bmp280Filter::COEFF_16);
    EXPECT_EQ(second.readings.size(), RECORD_COUNT);
}

void Bmp280ManagerTester ::testResetCommand() {
    this->tick(TICKS_TO_FIRST_READ);
    ASSERT_EQ(this->state(), State::RUN);
    const U32 resets = this->m_resets;

    this->sendCmd_RESET(TEST_INSTANCE_ID, 0);
    this->tick();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, Bmp280ManagerComponentBase::OPCODE_RESET, 0, Fw::CmdResponse::OK);
    // The command queued the error signal behind this tick, so it lands on the next one
    this->tick();
    ASSERT_EQ(this->state(), State::RESET);
    EXPECT_EQ(this->m_resets, resets + 1);
    this->tick(TICKS_TO_FIRST_READ - 1);
    ASSERT_EQ(this->state(), State::RUN);
}

// ----------------------------------------------------------------------
// Fake BMP280 on the I2C bus
// ----------------------------------------------------------------------

Drv::I2cStatus Bmp280ManagerTester ::from_busWrite_handler(FwIndexType portNum, U32 addr, Fw::Buffer& serBuffer) {
    if (this->m_busDown || addr != this->m_deviceAddress) {
        return Drv::I2cStatus::I2C_WRITE_ERR;
    }
    // [register, value, register, value, ...]: auto-increment is not used for writes
    const U8* data = serBuffer.getData();
    const FwSizeType size = serBuffer.getSize();
    EXPECT_EQ(size % 2, 0u) << "BMP280 writes are register/value pairs";
    for (FwSizeType i = 0; i + 1 < size; i += 2) {
        const U8 reg = data[i];
        const U8 value = data[i + 1];
        if (reg == REG_RESET && value == RESET_COMMAND) {
            this->m_resets++;
            this->m_regs[REG_CTRL_MEAS] = 0x00;
            this->m_regs[REG_CONFIG] = 0x00;
        } else {
            this->m_regs[reg] = value;
        }
    }
    return Drv::I2cStatus::I2C_OK;
}

Drv::I2cStatus Bmp280ManagerTester ::from_busWriteRead_handler(FwIndexType portNum,
                                                               U32 addr,
                                                               Fw::Buffer& writeBuffer,
                                                               Fw::Buffer& readBuffer) {
    if (this->m_busDown || addr != this->m_deviceAddress) {
        return Drv::I2cStatus::I2C_READ_ERR;
    }
    EXPECT_EQ(writeBuffer.getSize(), 1u);
    const U8 start = writeBuffer.getData()[0];
    const FwSizeType size = readBuffer.getSize();
    EXPECT_LE(start + size, this->m_regs.size());
    std::copy_n(this->m_regs.begin() + start, size, readBuffer.getData());
    return Drv::I2cStatus::I2C_OK;
}

Fw::Success::T Bmp280ManagerTester ::productGet_handler(FwDpIdType id, FwSizeType size, Fw::Buffer& buffer) {
    // `size` is the whole packet size, as the DataProducts buffer manager sees it
    this->pushProductGetEntry(id, size);
    this->m_dpStorage.emplace_back(size);
    std::vector<U8>& storage = this->m_dpStorage.back();
    buffer = Fw::Buffer(storage.data(), storage.size());
    return Fw::Success::SUCCESS;
}

void Bmp280ManagerTester ::productSend_handler(FwDpIdType id, const Fw::Buffer& buffer) {
    this->pushProductSendEntry(id, buffer);
    this->m_sentProducts.emplace_back(buffer.getData(), buffer.getData() + buffer.getSize());
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void Bmp280ManagerTester ::tick(U32 count) {
    for (U32 i = 0; i < count; i++) {
        this->invoke_to_run(0, 0);
    }
}

Bmp280ManagerTester::Decoded Bmp280ManagerTester ::fillContainer() {
    this->tick(TICKS_TO_FIRST_READ - 1);
    for (FwSizeType i = 0; i < RECORD_COUNT; i++) {
        this->tick();
        // Keep the telemetry history from filling up
        if (i % 100 == 99) {
            this->clearTlm();
        }
    }
    EXPECT_EQ(this->m_sentProducts.size(), 1u);
    return this->decodeSent(0);
}

Bmp280ManagerTester::Decoded Bmp280ManagerTester ::decodeSent(FwSizeType index) {
    EXPECT_LT(index, this->m_sentProducts.size());
    if (index >= this->m_sentProducts.size()) {
        return Decoded();
    }
    // Deserialize the header to find the data size, then decode the records
    std::vector<U8>& sent = this->m_sentProducts[index];
    Fw::DpContainer container;
    container.setBuffer(Fw::Buffer(sent.data(), sent.size()));
    EXPECT_EQ(container.deserializeHeader(), Fw::FW_SERIALIZE_OK);
    return this->decode(sent.data() + Fw::DpContainer::DATA_OFFSET, container.getDataSize());
}

Bmp280ManagerTester::State Bmp280ManagerTester ::state() const {
    return static_cast<const Bmp280ManagerComponentBase&>(this->component).sensorSm_getState();
}

void Bmp280ManagerTester ::setAdc(U32 adcP, U32 adcT) {
    // 20-bit values: msb[19:12] lsb[11:4] xlsb[7:4]
    this->m_regs[REG_PRESS_MSB] = static_cast<U8>(adcP >> 12);
    this->m_regs[REG_PRESS_MSB + 1] = static_cast<U8>(adcP >> 4);
    this->m_regs[REG_PRESS_MSB + 2] = static_cast<U8>((adcP & 0x0F) << 4);
    this->m_regs[REG_PRESS_MSB + 3] = static_cast<U8>(adcT >> 12);
    this->m_regs[REG_PRESS_MSB + 4] = static_cast<U8>(adcT >> 4);
    this->m_regs[REG_PRESS_MSB + 5] = static_cast<U8>((adcT & 0x0F) << 4);
}

Bmp280ManagerTester::Decoded Bmp280ManagerTester ::decode(const U8* data, FwSizeType size) const {
    Decoded decoded;
    Fw::ExternalSerializeBuffer serialBuffer(const_cast<U8*>(data), size);
    EXPECT_EQ(serialBuffer.setBuffLen(size), Fw::FW_SERIALIZE_OK);

    const FwDpIdType baseId = this->component.getIdBase();
    FwDpIdType recordId = 0;
    EXPECT_EQ(serialBuffer.deserializeTo(recordId), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(recordId, baseId + Bmp280ManagerComponentBase::RecordId::CalibRecord);
    EXPECT_EQ(serialBuffer.deserializeTo(decoded.calib), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(serialBuffer.deserializeTo(recordId), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(recordId, baseId + Bmp280ManagerComponentBase::RecordId::ConfigRecord);
    EXPECT_EQ(serialBuffer.deserializeTo(decoded.config), Fw::FW_SERIALIZE_OK);

    while (serialBuffer.getDeserializeSizeLeft() > 0) {
        EXPECT_EQ(serialBuffer.deserializeTo(recordId), Fw::FW_SERIALIZE_OK);
        EXPECT_EQ(recordId, baseId + Bmp280ManagerComponentBase::RecordId::ReadingRecord);
        Bmp280DataTimed record;
        EXPECT_EQ(serialBuffer.deserializeTo(record), Fw::FW_SERIALIZE_OK);
        decoded.readings.push_back(record);
    }
    return decoded;
}

}  // namespace Mara
