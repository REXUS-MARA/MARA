// ======================================================================
// \title  FatalHandlerTester.hpp
// \author fsowa
// \brief  hpp file for FatalHandler component test harness implementation class
// ======================================================================

#ifndef Mara_FatalHandlerTester_HPP
#define Mara_FatalHandlerTester_HPP

#include "Mara/Components/FatalHandler/FatalHandler.hpp"
#include "Mara/Components/FatalHandler/FatalHandlerGTestBase.hpp"

namespace Mara {

class FatalHandlerTester final : public FatalHandlerGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 100;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object FatalHandlerTester
    FatalHandlerTester();

    //! Destroy object FatalHandlerTester
    ~FatalHandlerTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! To do
    //! A FATAL is logged as a warning with its ID and count, and counted in telemetry
    void testFatalIsSurvived();

    //! Warnings stop after the throttle (20); the count keeps going
    void testThrottledButCounted();

  private:
    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    FatalHandler component;
};

}  // namespace Mara

#endif
