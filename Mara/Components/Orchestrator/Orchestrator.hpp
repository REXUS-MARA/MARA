// ======================================================================
// \title  Orchestrator.hpp
// \author fsowa
// \brief  hpp file for Orchestrator component implementation class
// ======================================================================

#ifndef Mara_Orchestrator_HPP
#define Mara_Orchestrator_HPP

#include "Mara/Components/Orchestrator/OrchestratorComponentAc.hpp"

namespace Mara {

class Orchestrator final : public OrchestratorComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct Orchestrator object
    Orchestrator(const char* const compName  //!< The component name
    );

    //! Destroy Orchestrator object
    ~Orchestrator();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for EODSHigh
    void EODSHigh_handler(FwIndexType portNum  //!< The port number
                          ) override;

    //! Handler implementation for LOHigh
    void LOHigh_handler(FwIndexType portNum  //!< The port number
                        ) override;

    //! Handler implementation for SOEHigh
    void SOEHigh_handler(FwIndexType portNum  //!< The port number
                         ) override;

    //! Handler implementation for schedIn
    //!
    //! 1 Hz tick, drives the experiment timeline
    void schedIn_handler(FwIndexType portNum,  //!< The port number
                         U32 context           //!< The call order
                         ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command enterTestMode
    void enterTestMode_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                  U32 cmdSeq            //!< The command sequence number
                                  ) override;

    //! Handler implementation for command exitTestMode
    void exitTestMode_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                 U32 cmdSeq            //!< The command sequence number
                                 ) override;

    //! Handler implementation for command testDrillON
    //!
    //! TEST mode: start the drill
    void testDrillON_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                U32 cmdSeq            //!< The command sequence number
                                ) override;

    //! Handler implementation for command testDrillOFF
    //!
    //! TEST mode: stop the drill
    void testDrillOFF_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                 U32 cmdSeq            //!< The command sequence number
                                 ) override;

    //! Handler implementation for command testPlatformMoveTo
    //!
    //! TEST mode: move the platform to an absolute position in encoder counts.
    //! PlatformMotor clamps it to [MIN_POSITION, MAX_POSITION]. 0 is fully down.
    void testPlatformMoveTo_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                       U32 cmdSeq,           //!< The command sequence number
                                       I32 position) override;

    //! Handler implementation for command testPlatformStop
    //!
    //! TEST mode: halt the platform where it is
    void testPlatformStop_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                     U32 cmdSeq            //!< The command sequence number
                                     ) override;

    //! Handler implementation for command testOpticalCameraON
    void testOpticalCameraON_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                        U32 cmdSeq            //!< The command sequence number
                                        ) override;

    //! Handler implementation for command testOpticalCameraOFF
    void testOpticalCameraOFF_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                         U32 cmdSeq            //!< The command sequence number
                                         ) override;

    //! Handler implementation for command testThermalCameraON
    void testThermalCameraON_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                        U32 cmdSeq            //!< The command sequence number
                                        ) override;

    //! Handler implementation for command testThermalCameraOFF
    void testThermalCameraOFF_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                         U32 cmdSeq            //!< The command sequence number
                                         ) override;

  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine actions
    // ----------------------------------------------------------------------

    //! Implementation for action drillOn of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_drillOn(SmId smId,  //!< The state machine id
                                                      Mara_OrchestratorStateMachine::Signal signal  //!< The signal
                                                      ) override;

    //! Implementation for action drillOff of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_drillOff(SmId smId,  //!< The state machine id
                                                       Mara_OrchestratorStateMachine::Signal signal  //!< The signal
                                                       ) override;

    //! Implementation for action cameraOn of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_cameraOn(SmId smId,  //!< The state machine id
                                                       Mara_OrchestratorStateMachine::Signal signal  //!< The signal
                                                       ) override;

    //! Implementation for action cameraOff of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_cameraOff(SmId smId,  //!< The state machine id
                                                        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
                                                        ) override;

    //! Implementation for action platformEnable of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_platformEnable(
        SmId smId,                                    //!< The state machine id
        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action platformAdvance of state machine Mara_OrchestratorStateMachine
    //!
    //! Move the platform up to DRILL_POSITION
    void Mara_OrchestratorStateMachine_action_platformAdvance(
        SmId smId,                                    //!< The state machine id
        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action platformRetract of state machine Mara_OrchestratorStateMachine
    //!
    //! Move the platform back down to 0
    void Mara_OrchestratorStateMachine_action_platformRetract(
        SmId smId,                                    //!< The state machine id
        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action platformStop of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_platformStop(SmId smId,  //!< The state machine id
                                                           Mara_OrchestratorStateMachine::Signal signal  //!< The signal
                                                           ) override;

    //! Implementation for action doTestPlatformMoveTo of state machine Mara_OrchestratorStateMachine
    //!
    //! test the platform: move to an absolute position
    void Mara_OrchestratorStateMachine_action_doTestPlatformMoveTo(
        SmId smId,                                     //!< The state machine id
        Mara_OrchestratorStateMachine::Signal signal,  //!< The signal
        I32 value                                      //!< The value
        ) override;

    //! Implementation for action doTestThermalCameraON of state machine Mara_OrchestratorStateMachine
    //!
    //! test the thermal camera ON
    void Mara_OrchestratorStateMachine_action_doTestThermalCameraON(
        SmId smId,                                    //!< The state machine id
        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action doTestThermalCameraOFF of state machine Mara_OrchestratorStateMachine
    //!
    //! test the thermal camera OFF
    void Mara_OrchestratorStateMachine_action_doTestThermalCameraOFF(
        SmId smId,                                    //!< The state machine id
        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action doTestLO of state machine Mara_OrchestratorStateMachine
    //!
    //! test the LO
    void Mara_OrchestratorStateMachine_action_doTestLO(SmId smId,  //!< The state machine id
                                                       Mara_OrchestratorStateMachine::Signal signal  //!< The signal
                                                       ) override;

    //! Implementation for action doTestSOE of state machine Mara_OrchestratorStateMachine
    //!
    //! test the SOE
    void Mara_OrchestratorStateMachine_action_doTestSOE(SmId smId,  //!< The state machine id
                                                        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
                                                        ) override;

    //! Implementation for action doTestEODS of state machine Mara_OrchestratorStateMachine
    //!
    //! test the EODS
    void Mara_OrchestratorStateMachine_action_doTestEODS(SmId smId,  //!< The state machine id
                                                         Mara_OrchestratorStateMachine::Signal signal  //!< The signal
                                                         ) override;

    //! Implementation for action loadTimeline of state machine Mara_OrchestratorStateMachine
    //!
    //! Read the timeline parameters once, so the running timeline can't change
    void Mara_OrchestratorStateMachine_action_loadTimeline(SmId smId,  //!< The state machine id
                                                           Mara_OrchestratorStateMachine::Signal signal  //!< The signal
                                                           ) override;

    //! Implementation for action resetPhaseTimer of state machine Mara_OrchestratorStateMachine
    //!
    //! Restart the per-phase seconds counter
    void Mara_OrchestratorStateMachine_action_resetPhaseTimer(
        SmId smId,                                    //!< The state machine id
        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action notifyEnterTEST of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_notifyEnterTEST(
        SmId smId,                                    //!< The state machine id
        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action notifyExitTEST of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_notifyExitTEST(
        SmId smId,                                    //!< The state machine id
        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action notifyEnterFlight of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_notifyEnterFlight(
        SmId smId,                                    //!< The state machine id
        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action notifyEnterExperiment of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_notifyEnterExperiment(
        SmId smId,                                    //!< The state machine id
        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action notifySpinUp of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_notifySpinUp(SmId smId,  //!< The state machine id
                                                           Mara_OrchestratorStateMachine::Signal signal  //!< The signal
                                                           ) override;

    //! Implementation for action notifyAdvance of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_notifyAdvance(
        SmId smId,                                    //!< The state machine id
        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action notifyRetract of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_notifyRetract(
        SmId smId,                                    //!< The state machine id
        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action notifyDone of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_notifyDone(SmId smId,  //!< The state machine id
                                                         Mara_OrchestratorStateMachine::Signal signal  //!< The signal
                                                         ) override;

    //! Implementation for action notifyEnterAfterExperiment of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_notifyEnterAfterExperiment(
        SmId smId,                                    //!< The state machine id
        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action notifyEnterSafe of state machine Mara_OrchestratorStateMachine
    void Mara_OrchestratorStateMachine_action_notifyEnterSafe(
        SmId smId,                                    //!< The state machine id
        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
        ) override;

  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine guards
    // ----------------------------------------------------------------------

    //! Implementation for guard spinUpDone of state machine Mara_OrchestratorStateMachine
    //!
    //! SPIN_UP_SECONDS have passed in SPIN_UP
    bool Mara_OrchestratorStateMachine_guard_spinUpDone(SmId smId,  //!< The state machine id
                                                        Mara_OrchestratorStateMachine::Signal signal  //!< The signal
    ) const override;

    //! Implementation for guard advanceDone of state machine Mara_OrchestratorStateMachine
    //!
    //! ADVANCE_SECONDS have passed in ADVANCE
    bool Mara_OrchestratorStateMachine_guard_advanceDone(SmId smId,  //!< The state machine id
                                                         Mara_OrchestratorStateMachine::Signal signal  //!< The signal
    ) const override;

    //! Implementation for guard retractDone of state machine Mara_OrchestratorStateMachine
    //!
    //! RETRACT_SECONDS have passed in RETRACTING (only marks the end of the timeline)
    bool Mara_OrchestratorStateMachine_guard_retractDone(SmId smId,  //!< The state machine id
                                                         Mara_OrchestratorStateMachine::Signal signal  //!< The signal
    ) const override;

  private:
    // ----------------------------------------------------------------------
    // Experiment timeline state
    // ----------------------------------------------------------------------

    U32 m_phaseSeconds = 0;  //!< Seconds since the current experiment phase began

    // Timeline parameters, loaded on entry to EXPERIMENT so a running timeline can't change
    U32 m_spinUpSeconds = 0;
    U32 m_advanceSeconds = 0;
    U32 m_retractSeconds = 0;
    I32 m_drillPosition = 0;
};

}  // namespace Mara

#endif
