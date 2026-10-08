// ======================================================================
// \title  GPIOWatcherTester.hpp
// \author fsowa
// \brief  hpp file for GPIOWatcher component test harness implementation class
// ======================================================================

#ifndef Mara_GPIOWatcherTester_HPP
#define Mara_GPIOWatcherTester_HPP

#include "Mara/Components/GPIOWatcher/GPIOWatcher.hpp"
#include "Mara/Components/GPIOWatcher/GPIOWatcherGTestBase.hpp"

#include <array>

namespace Mara {

class GPIOWatcherTester final : public GPIOWatcherGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 100;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    //! The three REXUS service-module lines
    enum Line { LO = 0, SOE = 1, EODS = 2 };

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object GPIOWatcherTester
    GPIOWatcherTester();

    //! Destroy object GPIOWatcherTester
    ~GPIOWatcherTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! A line held HIGH for 4 ticks fires once, and not again while held
    void testDetect(Line line);

    //! 1 to 3 HIGH ticks followed by LOW is a spike, with no signal
    void testSpike(Line line);

    //! After going LOW the line can fire again; LOW right after firing is not a spike
    void testRefire(Line line);

    //! The three lines are debounced independently
    void testIndependentLines();

    //! Failed pin reads (no GPIO hardware) never fire
    void testReadFailureNeverFires();

  private:
    // ----------------------------------------------------------------------
    // Handlers for typed from ports
    // ----------------------------------------------------------------------

    Drv::GpioStatus from_LOPinRead_handler(FwIndexType portNum, Fw::Logic& state) override;
    Drv::GpioStatus from_SOEPinRead_handler(FwIndexType portNum, Fw::Logic& state) override;
    Drv::GpioStatus from_EODSPinRead_handler(FwIndexType portNum, Fw::Logic& state) override;

  private:
    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

    //! Set a line's level for the following ticks
    void set(Line line, Fw::Logic level);

    //! Run the 20 Hz schedule count times
    void tick(U32 count = 1);

    //! Answer one pin read from the scripted level and status
    Drv::GpioStatus readPin(Line line, Fw::Logic& state) const;

    //! Signals and events per line
    U32 signals(Line line) const;
    FwSizeType detected(Line line) const;
    FwSizeType spikes(Line line) const;

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    GPIOWatcher component;

    //! Scripted pin levels
    std::array<Fw::Logic, 3> m_level{{Fw::Logic::LOW, Fw::Logic::LOW, Fw::Logic::LOW}};

    //! Status returned by every pin read
    Drv::GpioStatus m_readStatus = Drv::GpioStatus::OP_OK;
};

}  // namespace Mara

#endif
