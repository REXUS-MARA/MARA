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

    //! Handler implementation for command testDrill
    //!
    //! I'm not sure if those should be sync or async
    //! Because the test will take some time
    //! But do we expect it to end within some finite time
    //! It's a pickle for me
    void testDrill_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                              U32 cmdSeq            //!< The command sequence number
                              ) override;

    //! Handler implementation for command testMotor
    void testPlatform_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                              U32 cmdSeq            //!< The command sequence number
                              ) override;

  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine actions
    // ----------------------------------------------------------------------

    //! Implementation for action doTestDrill of state machine Mara_OrchestratorStateMachine
    //!
    //! test the main drill
    void Mara_OrchestratorStateMachine_action_doTestDrill(SmId smId,  //!< The state machine id
                                                          Mara_OrchestratorStateMachine::Signal signal  //!< The signal
                                                          ) override;

    //! Implementation for action doTestPlatform of state machine Mara_OrchestratorStateMachine
    //!
    //! test the platform motors
    void Mara_OrchestratorStateMachine_action_doTestPlatform(
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
};

}  // namespace Mara

#endif
