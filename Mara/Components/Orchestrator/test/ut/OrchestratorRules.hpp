// ======================================================================
// \title  OrchestratorRules.hpp
// \brief  STest rules for random testing of the Orchestrator state machine
//
// Each rule applies one input (a GPIO signal, ticks, or a command) to the
// Orchestrator and then checks the safety invariants in
// OrchestratorTester::checkStep().
// ======================================================================

#ifndef Mara_OrchestratorRules_HPP
#define Mara_OrchestratorRules_HPP

#include "OrchestratorTester.hpp"
#include "STest/Rule/Rule.hpp"

namespace Mara {

namespace OrchestratorRules {

//! Lift-off line goes high
struct LiftOff : public STest::Rule<OrchestratorTester> {
    LiftOff();
    bool precondition(const OrchestratorTester& state) override;
    void action(OrchestratorTester& state) override;
};

//! Start-of-experiment line goes high
struct StartOfExperiment : public STest::Rule<OrchestratorTester> {
    StartOfExperiment();
    bool precondition(const OrchestratorTester& state) override;
    void action(OrchestratorTester& state) override;
};

//! End-of-data-storage line goes high
struct EndOfDataStorage : public STest::Rule<OrchestratorTester> {
    EndOfDataStorage();
    bool precondition(const OrchestratorTester& state) override;
    void action(OrchestratorTester& state) override;
};

//! 1 to 20 rate group ticks, each checked
struct Ticks : public STest::Rule<OrchestratorTester> {
    Ticks();
    bool precondition(const OrchestratorTester& state) override;
    void action(OrchestratorTester& state) override;
};

//! enterTestMode command
struct EnterTest : public STest::Rule<OrchestratorTester> {
    EnterTest();
    bool precondition(const OrchestratorTester& state) override;
    void action(OrchestratorTester& state) override;
};

//! exitTestMode command
struct ExitTest : public STest::Rule<OrchestratorTester> {
    ExitTest();
    bool precondition(const OrchestratorTester& state) override;
    void action(OrchestratorTester& state) override;
};

//! A random test* command with a random position
struct TestCommand : public STest::Rule<OrchestratorTester> {
    TestCommand();
    bool precondition(const OrchestratorTester& state) override;
    void action(OrchestratorTester& state) override;
};

}  // namespace OrchestratorRules

//! Run `runs` independent random scenarios of `steps` steps each, each on a fresh
//! component with a random timeline. Returns the total number of steps taken.
U32 runRandomScenarios(U32 runs, U32 steps);

}  // namespace Mara

#endif
