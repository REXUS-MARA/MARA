// ======================================================================
// \title  LtrManagerTestMain.cpp
// \author fsowa
// \brief  cpp file for LtrManager component test main function
// ======================================================================

#include "LtrManagerTester.hpp"

TEST(Nominal, toDo) {
    Mara::LtrManagerTester tester;
    tester.toDo();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
