// ======================================================================
// \title  RustTesterTester.hpp
// \author lestarch-autobot
// \brief  hpp file for RustTester component test harness implementation class
// ======================================================================

#ifndef FprimeRustReference_RustTesterTester_HPP
#define FprimeRustReference_RustTesterTester_HPP

#include "FprimeRustReference/Components/RustTester/RustTester.hpp"
#include "FprimeRustReference/Components/RustTester/RustTesterGTestBase.hpp"

namespace FprimeRustReference {

class RustTesterTester final : public RustTesterGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs: one window is up to
    // 8 CoverageChunk + CoverageObject + CoverageDumpDone
    static const FwSizeType MAX_HISTORY_SIZE = 64;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object RustTesterTester
    RustTesterTester();

    //! Destroy object RustTesterTester
    ~RustTesterTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! UT-RT-1: MARK(id) emits Marker(id) synchronously and responds OK
    void markEmitsMarker();

    //! UT-RT-2: count > 8 is a VALIDATION_ERROR with no events
    void windowTooLargeRejected();

    //! UT-RT-3: uninstrumented image: DUMP_COVERAGE emits CoverageUnavailable and responds OK
    void uninstrumentedResponds();

    //! UT-RT-4: instrumented: paged windows reassemble to the object byte counts and the Done CRC
    void pagedWindowsReassemble();

    //! UT-RT-5: a window that does not start at chunk 0 without a snapshot is an EXECUTION_ERROR
    void windowWithoutSnapshotRejected();

    //! UT-RT-6: count = 0 zeroes the counters (arc counters of the next dump are zero) and responds OK
    void zeroResetsCounters();

    //! UT-RT-7: crc32 known answer (CRC-32/IEEE check value)
    void crc32KnownAnswer();

    //! UT-RT-8: white-box paging of an injected snapshot (every build)
    void injectedSnapshotWindows();

    //! UT-RT-9: MARK flushes the counters to the on-disk .gcda; count = 0 deletes it (instrumented build)
    void markFlushesAndResetUnlinks();

    //! UT-RT-10: BURST_CMD_U32 count bounds (0 and 17 rejected, 1 and 16 accepted) and copy count
    void burstCountBounds();

    //! UT-RT-11: burst packet bytes against Fw::CmdPacket (descriptor, opcode, U32 arg) and a raw byte KAT
    void burstPacketKnownAnswer();

    //! UT-RT-12: burst responses are counted per outcome and reset by the next burst
    void burstResponseCounting();

  private:
    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

    //! Send DUMP_COVERAGE and expect `response`
    void dump(U16 start, U8 count, Fw::CmdResponse::T response);

    //! Walk every window from chunk 0 until CoverageDumpDone; returns the chunks seen
    U32 walkAll(U32& objects, U32& totalBytes, U32& crc, U32& crcOfChunks);

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    RustTester component;

    //! Command sequence counter
    U32 m_cmdSeq;
};

}  // namespace FprimeRustReference

#endif
