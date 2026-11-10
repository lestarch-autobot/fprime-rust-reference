// ======================================================================
// \title  RustMonitorTestMain.cpp
// \author ubuntu
// \brief  cpp file for RustMonitor component test main function
// ======================================================================

#include "RustMonitorTester.hpp"


TEST(Nominal, UT_RM_1_setLimit) { FprimeRustReference::RustMonitorTester().setLimit(); }
TEST(Nominal, UT_RM_2_setLimitZeroRejected) { FprimeRustReference::RustMonitorTester().setLimitZeroRejected(); }
TEST(Nominal, UT_RM_3_countsAgainstLimit) { FprimeRustReference::RustMonitorTester().countsAgainstLimit(); }
TEST(Nominal, UT_RM_4_runDrainsQueue) { FprimeRustReference::RustMonitorTester().runDrainsQueue(); }
TEST(Nominal, UT_RM_5_setLimitBusyOnFullQueue) { FprimeRustReference::RustMonitorTester().setLimitBusyOnFullQueue(); }

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
