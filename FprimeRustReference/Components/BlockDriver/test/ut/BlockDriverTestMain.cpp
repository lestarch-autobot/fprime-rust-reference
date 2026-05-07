// ======================================================================
// \title  BlockDriverTestMain.cpp
// \author root
// \brief  cpp file for BlockDriver component test main function
// ======================================================================

#include "BlockDriverTester.hpp"

TEST(Nominal, testDataLoopBack) {
    FprimeRustReference::BlockDriverTester tester;
    tester.testDataLoopBack();
}

TEST(Nominal, testPing) {
    FprimeRustReference::BlockDriverTester tester;
    tester.testPing();
}

TEST(Nominal, testCycleIncrement) {
    FprimeRustReference::BlockDriverTester tester;
    tester.testCycleIncrement();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
