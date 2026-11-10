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
TEST(Nominal, UT_RT_7_crc32KnownAnswer) { FprimeRustReference::RustTesterTester().crc32KnownAnswer(); }
TEST(Nominal, UT_RT_8_injectedSnapshotWindows) { FprimeRustReference::RustTesterTester().injectedSnapshotWindows(); }
TEST(Nominal, UT_RT_9_markFlushesAndResetUnlinks) { FprimeRustReference::RustTesterTester().markFlushesAndResetUnlinks(); }
TEST(Nominal, UT_RT_10_burstCountBounds) { FprimeRustReference::RustTesterTester().burstCountBounds(); }
TEST(Nominal, UT_RT_11_burstPacketKnownAnswer) { FprimeRustReference::RustTesterTester().burstPacketKnownAnswer(); }
TEST(Nominal, UT_RT_12_burstResponseCounting) { FprimeRustReference::RustTesterTester().burstResponseCounting(); }

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
