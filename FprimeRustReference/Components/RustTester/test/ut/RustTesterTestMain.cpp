// ======================================================================
// \title  RustTesterTestMain.cpp
// \author lestarch-autobot
// \brief  cpp file for RustTester component test main function
// ======================================================================

#include "RustTesterTester.hpp"

TEST(Nominal, UT_RT_1_markEmitsMarker) { FprimeRustReference::RustTesterTester().markEmitsMarker(); }
TEST(Nominal, UT_RT_2_windowTooLargeRejected) { FprimeRustReference::RustTesterTester().windowTooLargeRejected(); }
TEST(Nominal, UT_RT_3_uninstrumentedResponds) { FprimeRustReference::RustTesterTester().uninstrumentedResponds(); }
TEST(Nominal, UT_RT_4_pagedWindowsReassemble) { FprimeRustReference::RustTesterTester().pagedWindowsReassemble(); }
TEST(Nominal, UT_RT_5_windowWithoutSnapshotRejected) { FprimeRustReference::RustTesterTester().windowWithoutSnapshotRejected(); }
TEST(Nominal, UT_RT_6_zeroResetsCounters) { FprimeRustReference::RustTesterTester().zeroResetsCounters(); }

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
