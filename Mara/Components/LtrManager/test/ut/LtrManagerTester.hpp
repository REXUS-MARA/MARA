// ======================================================================
// \title  LtrManagerTester.hpp
// \author fsowa
// \brief  hpp file for LtrManager component test harness implementation class
// ======================================================================

#ifndef Mara_LtrManagerTester_HPP
#define Mara_LtrManagerTester_HPP

#include "Mara/Components/LtrManager/LtrManager.hpp"
#include "Mara/Components/LtrManager/LtrManagerGTestBase.hpp"

namespace Mara {

class LtrManagerTester final : public LtrManagerGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 10;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    // Queue depth supplied to the component instance under test
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object LtrManagerTester
    LtrManagerTester();

    //! Destroy object LtrManagerTester
    ~LtrManagerTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! To do
    void toDo();

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
    LtrManager component;
};

}  // namespace Mara

#endif
