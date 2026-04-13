// ======================================================================
// \title  LtrManagerTester.cpp
// \author fsowa
// \brief  cpp file for LtrManager component test harness implementation class
// ======================================================================

#include "LtrManagerTester.hpp"

namespace Mara {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

LtrManagerTester ::LtrManagerTester()
    : LtrManagerGTestBase("LtrManagerTester", LtrManagerTester::MAX_HISTORY_SIZE), component("LtrManager") {
    this->initComponents();
    this->connectPorts();
}

LtrManagerTester ::~LtrManagerTester() {}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void LtrManagerTester ::toDo() {
    // TODO
}

}  // namespace Mara
