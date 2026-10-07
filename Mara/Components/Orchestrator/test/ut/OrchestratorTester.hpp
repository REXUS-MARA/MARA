// ======================================================================
// \title  OrchestratorTester.hpp
// \author fsowa
// \brief  hpp file for Orchestrator component test harness implementation class
// ======================================================================

#ifndef Mara_OrchestratorTester_HPP
#define Mara_OrchestratorTester_HPP

#include "Mara/Components/Orchestrator/Orchestrator.hpp"
#include "Mara/Components/Orchestrator/OrchestratorGTestBase.hpp"

namespace Mara {

class OrchestratorTester final : public OrchestratorGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 1000;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    // Queue depth supplied to the component instance under test
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

    using State = OrchestratorComponentBase::Mara_OrchestratorStateMachine::State;

    //! Every command the component accepts
    enum class Command {
        ENTER_TEST,
        EXIT_TEST,
        DRILL_ON,
        DRILL_OFF,
        PLATFORM_MOVE_TO,
        PLATFORM_STOP,
        CAMERA_ON,
        CAMERA_OFF,
        THERMAL_ON,
        THERMAL_OFF,
    };

    //! Kinds of step a random scenario can take
    enum class Step { LO, SOE, EODS, TICK, ENTER_TEST, EXIT_TEST, TEST_COMMAND };

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object OrchestratorTester
    OrchestratorTester();

    //! Destroy object OrchestratorTester
    ~OrchestratorTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! Boots in IDLE; LO -> FLIGHT; SOE -> EXPERIMENT with its entry actions
    void testFlightPath();

    //! Each phase ends on the exact tick its parameter says, with the right outputs
    void testTimelineTiming();

    //! Changing parameters during EXPERIMENT does not change the running timeline
    void testTimelineParamsLoadedOnce();

    //! EODS during SPIN_UP stops the drill and retracts
    void testEodsDuringSpinUpRetracts();

    //! EODS during ADVANCE stops the drill and retracts
    void testEodsDuringAdvanceRetracts();

    //! EODS after drilling does not retract a second time
    void testEodsAfterDrillingNoSecondRetract();

    //! DRILL_POSITION = 0 warns but the timeline still runs
    void testDrillPositionUnset();

    //! TEST entry, every test command's output, LO/SOE/EODS only logged, exit to a known state
    void testTestMode();

    //! Test commands outside TEST are rejected with an event and EXECUTION_ERROR
    void testCommandGating();

    //! enterTestMode only from IDLE, exitTestMode only from TEST
    void testEnterExitGating();

    //! Ticks outside EXPERIMENT change nothing
    void testTicksOutsideExperiment();

    //! The error signal leads to SAFE from FLIGHT; SAFE then ignores everything
    void testErrorToSafe();

    //! The error signal while drilling stops the drill and retracts on the way to SAFE
    void testErrorWhileDrillingRetracts();

    //! A full queue drops messages instead of asserting
    void testQueueOverflowDrops();

  public:
    // ----------------------------------------------------------------------
    // Helpers (also used by the STest rules)
    // ----------------------------------------------------------------------

    //! Clear all histories (public wrapper for the rules)
    void clearHistories() { this->clearHistory(); }

    //! Dispatch until the queue is empty (handlers queue state machine signals)
    void dispatchAll();

    //! Send a command, dispatch, and return its response
    Fw::CmdResponse command(Command cmd, I32 position = 0);

    //! Invoke an input port and dispatch
    void lo();
    void soe();
    void eods();
    void tick(U32 count = 1);

    //! Send the (otherwise unused) error signal and dispatch
    void error();

    //! Set the timeline parameters and load them
    void setTimeline(U32 spinUp, U32 advance, U32 retract, I32 drillPosition);

    //! Current state machine state
    State state() const;

    //! Whether a state is one of the DRILLING substates
    static bool isDrilling(State s);

    //! Whether a command is a test* command (gated to TEST mode)
    static bool isTestCommand(Command cmd);

    //! Total number of hardware output port invocations recorded
    FwSizeType hardwareOutputs() const;

    //! Check the safety invariants after one random step (STest rules).
    //! \param before the state before the step; histories were cleared before the step
    void checkStep(State before, Step step, Fw::CmdResponse response);

    //! Order of the flight states; TEST and SAFE are not part of the order
    static int flightRank(State s);

    //! The component under test (exposed for the rules)
    Orchestrator component;

  private:
    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

    //! Go from IDLE through FLIGHT into EXPERIMENT and clear histories
    void startExperiment();

    U32 m_cmdSeq = 0;
};

}  // namespace Mara

#endif
