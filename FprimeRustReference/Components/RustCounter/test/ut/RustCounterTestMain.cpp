// ======================================================================
// \title  RustCounterTestMain.cpp
// \author ubuntu
// \brief  cpp file for RustCounter component test main function
// ======================================================================

#include "RustCounterTester.hpp"


TEST(Nominal, UT_RC_1_resetAfterBump) { FprimeRustReference::RustCounterTester().resetAfterBump(); }
TEST(Nominal, UT_RC_2_bumpWithinAndPastThreshold) { FprimeRustReference::RustCounterTester().bumpWithinAndPastThreshold(); }
TEST(Nominal, UT_RC_3_bumpMaxNoWrap) { FprimeRustReference::RustCounterTester().bumpMaxNoWrap(); }
TEST(Nominal, UT_RC_4_thresholdUpdateTakesEffect) { FprimeRustReference::RustCounterTester().thresholdUpdateTakesEffect(); }
TEST(Nominal, UT_RC_5_ticks) { FprimeRustReference::RustCounterTester().ticks(); }

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
