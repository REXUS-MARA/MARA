// ======================================================================
// \title  GPIOWatcherTester.cpp
// \author fsowa
// \brief  cpp file for GPIOWatcher component test harness implementation class
// ======================================================================

#include "GPIOWatcherTester.hpp"

namespace Mara {

namespace {
//! Consecutive HIGH reads needed to accept a signal (GPIOWatcher's threshold)
constexpr U32 THRESHOLD = 4;
}  // namespace

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

GPIOWatcherTester ::GPIOWatcherTester()
    : GPIOWatcherGTestBase("GPIOWatcherTester", GPIOWatcherTester::MAX_HISTORY_SIZE), component("GPIOWatcher") {
    this->initComponents();
    this->connectPorts();
}

GPIOWatcherTester ::~GPIOWatcherTester() {}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void GPIOWatcherTester ::testDetect(Line line) {
    this->set(line, Fw::Logic::HIGH);

    this->tick(THRESHOLD - 1);
    ASSERT_EQ(this->signals(line), 0U);
    ASSERT_EQ(this->detected(line), 0U);

    this->tick();
    ASSERT_EQ(this->signals(line), 1U);
    ASSERT_EQ(this->detected(line), 1U);

    // Held HIGH: no repeat
    this->tick(20);
    ASSERT_EQ(this->signals(line), 1U);
    ASSERT_EQ(this->detected(line), 1U);
    ASSERT_EQ(this->spikes(line), 0U);
}

void GPIOWatcherTester ::testSpike(Line line) {
    FwSizeType expectedSpikes = 0;
    for (U32 highs = 1; highs < THRESHOLD; highs++) {
        this->set(line, Fw::Logic::HIGH);
        this->tick(highs);
        this->set(line, Fw::Logic::LOW);
        this->tick();
        expectedSpikes++;
        ASSERT_EQ(this->spikes(line), expectedSpikes) << highs << " HIGH ticks";
    }
    ASSERT_EQ(this->signals(line), 0U);

    // LOW with no preceding HIGH is not a spike
    this->tick(5);
    ASSERT_EQ(this->spikes(line), expectedSpikes);
}

void GPIOWatcherTester ::testRefire(Line line) {
    this->set(line, Fw::Logic::HIGH);
    this->tick(THRESHOLD);
    ASSERT_EQ(this->signals(line), 1U);

    // LOW right after firing: the counter was reset, so no spike
    this->set(line, Fw::Logic::LOW);
    this->tick();
    ASSERT_EQ(this->spikes(line), 0U);

    this->set(line, Fw::Logic::HIGH);
    this->tick(THRESHOLD);
    ASSERT_EQ(this->signals(line), 2U);
    ASSERT_EQ(this->detected(line), 2U);
}

void GPIOWatcherTester ::testIndependentLines() {
    // LO held HIGH, SOE toggling every tick (never 4 in a row), EODS LOW
    this->set(LO, Fw::Logic::HIGH);
    for (U32 i = 0; i < 12; i++) {
        this->set(SOE, (i % 2 == 0) ? Fw::Logic::HIGH : Fw::Logic::LOW);
        this->tick();
    }
    ASSERT_EQ(this->signals(LO), 1U);
    ASSERT_EQ(this->signals(SOE), 0U);
    ASSERT_EQ(this->signals(EODS), 0U);
    ASSERT_EQ(this->spikes(SOE), 6U);
    ASSERT_EQ(this->spikes(LO), 0U);
    ASSERT_EQ(this->spikes(EODS), 0U);
}

void GPIOWatcherTester ::testReadFailureNeverFires() {
    // No GPIO hardware: every read fails. The level the driver would report is ignored.
    m_readStatus = Drv::GpioStatus::NOT_OPENED;
    this->set(LO, Fw::Logic::HIGH);
    this->set(SOE, Fw::Logic::HIGH);
    this->set(EODS, Fw::Logic::HIGH);
    this->tick(50);

    ASSERT_EQ(this->signals(LO) + this->signals(SOE) + this->signals(EODS), 0U);
    ASSERT_EVENTS_SIZE(0);
}

// ----------------------------------------------------------------------
// Handlers for typed from ports
// ----------------------------------------------------------------------

Drv::GpioStatus GPIOWatcherTester ::from_LOPinRead_handler(FwIndexType portNum, Fw::Logic& state) {
    (void)portNum;
    return this->readPin(LO, state);
}

Drv::GpioStatus GPIOWatcherTester ::from_SOEPinRead_handler(FwIndexType portNum, Fw::Logic& state) {
    (void)portNum;
    return this->readPin(SOE, state);
}

Drv::GpioStatus GPIOWatcherTester ::from_EODSPinRead_handler(FwIndexType portNum, Fw::Logic& state) {
    (void)portNum;
    return this->readPin(EODS, state);
}

// ----------------------------------------------------------------------
// Helper functions
// ----------------------------------------------------------------------

void GPIOWatcherTester ::set(Line line, Fw::Logic level) {
    m_level[line] = level;
}

void GPIOWatcherTester ::tick(U32 count) {
    for (U32 i = 0; i < count; i++) {
        this->invoke_to_schedIn(0, 0);  // sync port: runs immediately
    }
}

Drv::GpioStatus GPIOWatcherTester ::readPin(Line line, Fw::Logic& state) const {
    if (m_readStatus != Drv::GpioStatus::OP_OK) {
        return m_readStatus;  // like a failed driver read: state left untouched
    }
    state = m_level[line];
    return Drv::GpioStatus::OP_OK;
}

U32 GPIOWatcherTester ::signals(Line line) const {
    switch (line) {
        case LO:
            return this->fromPortHistorySize_LOHigh;
        case SOE:
            return this->fromPortHistorySize_SOEHigh;
        default:
            return this->fromPortHistorySize_EODSHigh;
    }
}

FwSizeType GPIOWatcherTester ::detected(Line line) const {
    switch (line) {
        case LO:
            return this->eventsSize_LODetected;
        case SOE:
            return this->eventsSize_SOEDetected;
        default:
            return this->eventsSize_EODSDetected;
    }
}

FwSizeType GPIOWatcherTester ::spikes(Line line) const {
    switch (line) {
        case LO:
            return this->eventsSize_LOSpike;
        case SOE:
            return this->eventsSize_SOESpike;
        default:
            return this->eventsSize_EODSSpike;
    }
}

}  // namespace Mara
