// ======================================================================
// \title  RustTesterTester.cpp
// \author lestarch-autobot
// \brief  cpp file for RustTester component test harness implementation class
// ======================================================================

#include "RustTesterTester.hpp"

#include "Fw/Test/UnitTest.hpp"

namespace FprimeRustReference {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

RustTesterTester ::RustTesterTester()
    : RustTesterGTestBase("RustTesterTester", RustTesterTester::MAX_HISTORY_SIZE),
      component("RustTester"),
      m_cmdSeq(0) {
    this->initComponents();
    this->connectPorts();
}

RustTesterTester ::~RustTesterTester() {
    this->component.deinit();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void RustTesterTester ::markEmitsMarker() {
    REQUIREMENT("REQ-RustTester-001");
    COMMENT("UT-RT-1: MARK(42) emits Marker(42) before the OK response");
    this->clearHistory();
    this->sendCmd_MARK(0, 7, 42);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_Marker_SIZE(1);
    ASSERT_EVENTS_Marker(0, 42);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, RustTester::OPCODE_MARK, 7, Fw::CmdResponse::OK);
}

void RustTesterTester ::windowTooLargeRejected() {
    REQUIREMENT("REQ-RustTester-005");
    COMMENT("UT-RT-2: DUMP_COVERAGE(0, 9) is a VALIDATION_ERROR and emits nothing");
    this->clearHistory();
    this->dump(0, RustTester::MAX_CHUNKS_PER_WINDOW + 1, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_SIZE(0);
}

void RustTesterTester ::uninstrumentedResponds() {
    REQUIREMENT("REQ-RustTester-004");
    COMMENT("UT-RT-3: without a gcov information section every DUMP_COVERAGE answers CoverageUnavailable + OK");
    if (RustTester::coverageAvailable()) {
        GTEST_SKIP() << "built with FPRIME_RUST_REFERENCE_COVERAGE: instrumented image";
    }
    this->clearHistory();
    this->dump(0, 8, Fw::CmdResponse::OK);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CoverageUnavailable_SIZE(1);
    this->clearHistory();
    this->dump(0, 0, Fw::CmdResponse::OK);
    ASSERT_EVENTS_CoverageUnavailable_SIZE(1);
}

void RustTesterTester ::pagedWindowsReassemble() {
    REQUIREMENT("REQ-RustTester-002");
    REQUIREMENT("REQ-RustTester-003");
    COMMENT("UT-RT-4: windows of 8 from chunk 0 to Done: per-object byte counts match the chunk lengths, the Done "
            "CRC equals the CRC-32 over all chunk bytes, and a repeated walk is byte-identical (snapshot)");
    if (!RustTester::coverageAvailable()) {
        GTEST_SKIP() << "built without FPRIME_RUST_REFERENCE_COVERAGE: no gcov information section";
    }
    U32 objects = 0;
    U32 totalBytes = 0;
    U32 crc = 0;
    U32 crcOfChunks = 0;
    const U32 chunks = this->walkAll(objects, totalBytes, crc, crcOfChunks);
    ASSERT_GT(objects, 0U);
    ASSERT_GT(chunks, 0U);
    ASSERT_EQ(crc, crcOfChunks);
    // the window past the end answers Done again, nothing else
    this->clearHistory();
    this->dump(static_cast<U16>(chunks), 8, Fw::CmdResponse::OK);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CoverageDumpDone_SIZE(1);
    ASSERT_EVENTS_CoverageDumpDone(0, objects, totalBytes, crc);
}

void RustTesterTester ::windowWithoutSnapshotRejected() {
    REQUIREMENT("REQ-RustTester-003");
    COMMENT("UT-RT-5: after count = 0 the snapshot is gone: DUMP_COVERAGE(1, 8) is an EXECUTION_ERROR without events");
    if (!RustTester::coverageAvailable()) {
        GTEST_SKIP() << "built without FPRIME_RUST_REFERENCE_COVERAGE: no gcov information section";
    }
    this->clearHistory();
    this->dump(0, 0, Fw::CmdResponse::OK);
    this->dump(1, 8, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_SIZE(0);
}

void RustTesterTester ::zeroResetsCounters() {
    REQUIREMENT("REQ-RustTester-006");
    COMMENT("UT-RT-6: a walk, then count = 0, then a walk: same objects, a stream no longer than before (libgcov "
            "omits all-zero counter records) and a different CRC because the arc counters are zero");
    if (!RustTester::coverageAvailable()) {
        GTEST_SKIP() << "built without FPRIME_RUST_REFERENCE_COVERAGE: no gcov information section";
    }
    U32 objects1, bytes1, crc1, chunksCrc1;
    (void)this->walkAll(objects1, bytes1, crc1, chunksCrc1);
    this->clearHistory();
    this->dump(0, 0, Fw::CmdResponse::OK);
    ASSERT_EVENTS_SIZE(0);
    U32 objects2, bytes2, crc2, chunksCrc2;
    (void)this->walkAll(objects2, bytes2, crc2, chunksCrc2);
    ASSERT_EQ(objects1, objects2);
    ASSERT_LE(bytes2, bytes1);
    // this tester's own instrumented code ran between the walks: the first stream counted it, the zeroed one counts
    // only what ran after the reset (the walk itself is not instrumented), so the streams differ
    ASSERT_NE(crc1, crc2);
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void RustTesterTester ::dump(U16 start, U8 count, Fw::CmdResponse::T response) {
    const FwSizeType before = this->cmdResponseHistory->size();
    this->m_cmdSeq++;
    this->sendCmd_DUMP_COVERAGE(0, this->m_cmdSeq, start, count);
    ASSERT_CMD_RESPONSE_SIZE(before + 1);
    ASSERT_CMD_RESPONSE(before, RustTester::OPCODE_DUMP_COVERAGE, this->m_cmdSeq, Fw::CmdResponse(response));
}

U32 RustTesterTester ::walkAll(U32& objects, U32& totalBytes, U32& crc, U32& crcOfChunks) {
    U32 chunks = 0;
    U32 bytesSeen = 0;
    U32 objectsSeen = 0;
    U32 objectBytes = 0;
    crcOfChunks = 0;
    bool done = false;
    for (U32 window = 0; !done && window < 4096; window++) {
        this->clearHistory();
        this->dump(static_cast<U16>(chunks), RustTester::MAX_CHUNKS_PER_WINDOW, Fw::CmdResponse::OK);
        for (FwSizeType i = 0; i < this->eventHistory_CoverageChunk->size(); i++) {
            const EventEntry_CoverageChunk& e = this->eventHistory_CoverageChunk->at(i);
            EXPECT_EQ(e.object, objectsSeen);
            EXPECT_EQ(e.offset, objectBytes);
            EXPECT_GE(e.length, 1U);
            EXPECT_LE(e.length, RustTester::CHUNK_BYTES);
            const U32 words[8] = {e.data0, e.data1, e.data2, e.data3, e.data4, e.data5, e.data6, e.data7};
            U8 bytes[RustTester::CHUNK_BYTES];
            for (U32 b = 0; b < RustTester::CHUNK_BYTES; b++) {
                bytes[b] = static_cast<U8>(words[b / 4] >> (24 - 8 * (b % 4)));
            }
            crcOfChunks = RustTester::crc32(crcOfChunks, bytes, e.length);
            objectBytes += e.length;
            bytesSeen += e.length;
            chunks++;
            if (e.length < RustTester::CHUNK_BYTES ||
                (this->eventHistory_CoverageObject->size() > 0 &&
                 this->eventHistory_CoverageObject->at(this->eventHistory_CoverageObject->size() - 1).object ==
                     objectsSeen &&
                 this->eventHistory_CoverageObject->at(this->eventHistory_CoverageObject->size() - 1).bytes ==
                     objectBytes)) {
                objectsSeen++;
                objectBytes = 0;
            }
        }
        for (FwSizeType i = 0; i < this->eventHistory_CoverageObject->size(); i++) {
            const EventEntry_CoverageObject& o = this->eventHistory_CoverageObject->at(i);
            EXPECT_LT(o.object, objectsSeen);
            EXPECT_GT(o.bytes, 0U);
        }
        if (this->eventHistory_CoverageDumpDone->size() == 1) {
            const EventEntry_CoverageDumpDone& d = this->eventHistory_CoverageDumpDone->at(0);
            objects = d.objects;
            totalBytes = d.totalBytes;
            crc = d.crc;
            done = true;
        }
    }
    EXPECT_TRUE(done);
    EXPECT_EQ(objects, objectsSeen);
    EXPECT_EQ(totalBytes, bytesSeen);
    return chunks;
}

}  // namespace FprimeRustReference
