// ======================================================================
// \title  RelayTestMain.cpp
// \author ubuntu
// \brief  cpp file for Relay component test main function
// ======================================================================

#include "RelayTester.hpp"


TEST(Nominal, UT_Relay_1_forwardsCount) { FprimeRustReference::RelayTester().forwardsCount(); }
TEST(Nominal, UT_Relay_2_relayedTelemetry) { FprimeRustReference::RelayTester().relayedTelemetry(); }
TEST(Nominal, UT_Relay_2_unconnectedOutput) { FprimeRustReference::RelayTester().unconnectedOutput(); }

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
