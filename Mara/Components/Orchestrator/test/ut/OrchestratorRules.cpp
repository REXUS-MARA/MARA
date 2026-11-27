// ======================================================================
// \title  OrchestratorRules.cpp
// \brief  STest rules for random testing of the Orchestrator state machine
// ======================================================================

#include "OrchestratorRules.hpp"

#include "STest/Pick/Pick.hpp"
#include "STest/Scenario/BoundedScenario.hpp"
#include "STest/Scenario/RandomScenario.hpp"

namespace Mara {

namespace OrchestratorRules {

namespace {
using Step = OrchestratorTester::Step;
using Command = OrchestratorTester::Command;

const Command TEST_COMMANDS[] = {
    Command::DRILL_ON,  Command::DRILL_OFF,  Command::PLATFORM_MOVE_TO, Command::PLATFORM_STOP,
    Command::CAMERA_ON, Command::CAMERA_OFF, Command::THERMAL_ON,       Command::THERMAL_OFF,
};

//! Apply one input with cleared histories, then check the invariants
template <typename Input>
void step(OrchestratorTester& state, Step kind, Input input) {
    state.clearHistories();
    const OrchestratorTester::State before = state.state();
    const Fw::CmdResponse response = input();
    state.checkStep(before, kind, response);
}
}  // namespace

LiftOff::LiftOff() : STest::Rule<OrchestratorTester>("LiftOff") {}
bool LiftOff::precondition(const OrchestratorTester& state) {
    return true;
}
void LiftOff::action(OrchestratorTester& state) {
    step(state, Step::LO, [&state]() {
        state.lo();
        return Fw::CmdResponse::OK;
    });
}

StartOfExperiment::StartOfExperiment() : STest::Rule<OrchestratorTester>("StartOfExperiment") {}
bool StartOfExperiment::precondition(const OrchestratorTester& state) {
    return true;
}
void StartOfExperiment::action(OrchestratorTester& state) {
    step(state, Step::SOE, [&state]() {
        state.soe();
        return Fw::CmdResponse::OK;
    });
}

EndOfDataStorage::EndOfDataStorage() : STest::Rule<OrchestratorTester>("EndOfDataStorage") {}
bool EndOfDataStorage::precondition(const OrchestratorTester& state) {
    return true;
}
void EndOfDataStorage::action(OrchestratorTester& state) {
    step(state, Step::EODS, [&state]() {
        state.eods();
        return Fw::CmdResponse::OK;
    });
}

Ticks::Ticks() : STest::Rule<OrchestratorTester>("Ticks") {}
bool Ticks::precondition(const OrchestratorTester& state) {
    return true;
}
void Ticks::action(OrchestratorTester& state) {
    // Check every tick on its own, so a phase change is attributed to the right step
    const U32 count = STest::Pick::lowerUpper(1, 20);
    for (U32 i = 0; i < count; i++) {
        step(state, Step::TICK, [&state]() {
            state.tick();
            return Fw::CmdResponse::OK;
        });
    }
}

EnterTest::EnterTest() : STest::Rule<OrchestratorTester>("EnterTest") {}
bool EnterTest::precondition(const OrchestratorTester& state) {
    return true;
}
void EnterTest::action(OrchestratorTester& state) {
    step(state, Step::ENTER_TEST, [&state]() { return state.command(Command::ENTER_TEST); });
}

ExitTest::ExitTest() : STest::Rule<OrchestratorTester>("ExitTest") {}
bool ExitTest::precondition(const OrchestratorTester& state) {
    return true;
}
void ExitTest::action(OrchestratorTester& state) {
    step(state, Step::EXIT_TEST, [&state]() { return state.command(Command::EXIT_TEST); });
}

TestCommand::TestCommand() : STest::Rule<OrchestratorTester>("TestCommand") {}
bool TestCommand::precondition(const OrchestratorTester& state) {
    return true;
}
void TestCommand::action(OrchestratorTester& state) {
    const Command cmd = TEST_COMMANDS[STest::Pick::startLength(0, FW_NUM_ARRAY_ELEMENTS(TEST_COMMANDS))];
    const I32 position = static_cast<I32>(STest::Pick::lowerUpper(0, 10000)) - 5000;
    step(state, Step::TEST_COMMAND, [&state, cmd, position]() { return state.command(cmd, position); });
}

}  // namespace OrchestratorRules

U32 runRandomScenarios(U32 runs, U32 steps) {
    OrchestratorRules::LiftOff liftOff;
    OrchestratorRules::StartOfExperiment startOfExperiment;
    OrchestratorRules::EndOfDataStorage endOfDataStorage;
    OrchestratorRules::Ticks ticks;
    OrchestratorRules::EnterTest enterTest;
    OrchestratorRules::ExitTest exitTest;
    OrchestratorRules::TestCommand testCommand;
    STest::Rule<OrchestratorTester>* rules[] = {&liftOff, &startOfExperiment, &endOfDataStorage, &ticks,
                                                &enterTest, &exitTest, &testCommand};

    U32 total = 0;
    for (U32 run = 0; run < runs && !::testing::Test::HasFatalFailure(); run++) {
        // Fresh component, random timeline short enough for the ticks to get through it
        OrchestratorTester tester;
        tester.setTimeline(STest::Pick::lowerUpper(0, 3), STest::Pick::lowerUpper(0, 5), STest::Pick::lowerUpper(0, 3),
                           static_cast<I32>(STest::Pick::lowerUpper(0, 2000)) - 1000);

        STest::RandomScenario<OrchestratorTester> random("Random Orchestrator inputs", rules,
                                                         FW_NUM_ARRAY_ELEMENTS(rules));
        STest::BoundedScenario<OrchestratorTester> bounded("Bounded random scenario", random, steps);
        total += bounded.run(tester);
    }
    return total;
}

}  // namespace Mara
