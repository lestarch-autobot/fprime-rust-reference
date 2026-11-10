// ======================================================================
// \title  RustTesterTester.cpp
// \author lestarch-autobot
// \brief  cpp file for RustTester component test harness implementation class
// ======================================================================

#include "RustTesterTester.hpp"

#include <Fw/Cmd/CmdPacket.hpp>
#include <Fw/Com/ComPacket.hpp>

#include "Fw/Test/UnitTest.hpp"
#include "Os/FileSystem.hpp"

#include <cstring>

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
    this->dump(0, static_cast<U8>(RustTester::MAX_CHUNKS_PER_WINDOW + 1), Fw::CmdResponse::VALIDATION_ERROR);
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
            "CRC equals the CRC-32 over all chunk bytes, and a window repeated after 50 MARKs is byte-identical "
            "(snapshot)");
    if (!RustTester::coverageAvailable()) {
        GTEST_SKIP() << "built without FPRIME_RUST_REFERENCE_COVERAGE: no gcov information section";
    }
    U32 objects = 0;
    U32 totalBytes = 0;
    U32 crc = 0;
    U32 crcOfChunks = 0;
    const U32 chunks = this->walkAll(objects, totalBytes, crc, crcOfChunks);
    ASSERT_GT(objects, 0U);
    ASSERT_GE(chunks, 2U);
    ASSERT_EQ(crc, crcOfChunks);
    // the snapshot makes a repeated window byte-identical even though instrumented code (this component's
    // autocode, driven by MARK) ran in between
    const U16 again = static_cast<U16>(chunks / 2);
    this->clearHistory();
    this->dump(again, RustTester::MAX_CHUNKS_PER_WINDOW, Fw::CmdResponse::OK);
    const FwSizeType repeatChunks = this->eventHistory_CoverageChunk->size();
    ASSERT_GT(repeatChunks, 0U);
    U32 first[RustTester::MAX_CHUNKS_PER_WINDOW][8];
    for (FwSizeType i = 0; i < repeatChunks; i++) {
        const EventEntry_CoverageChunk& e = this->eventHistory_CoverageChunk->at(i);
        const U32 words[8] = {e.data0, e.data1, e.data2, e.data3, e.data4, e.data5, e.data6, e.data7};
        for (U32 w = 0; w < 8; w++) {
            first[i][w] = words[w];
        }
    }
    for (U32 i = 0; i < 50; i++) {
        this->sendCmd_MARK(0, 1000 + i, i);
    }
    this->clearHistory();
    this->dump(again, RustTester::MAX_CHUNKS_PER_WINDOW, Fw::CmdResponse::OK);
    ASSERT_EQ(this->eventHistory_CoverageChunk->size(), repeatChunks);
    for (FwSizeType i = 0; i < repeatChunks; i++) {
        const EventEntry_CoverageChunk& e = this->eventHistory_CoverageChunk->at(i);
        const U32 words[8] = {e.data0, e.data1, e.data2, e.data3, e.data4, e.data5, e.data6, e.data7};
        for (U32 w = 0; w < 8; w++) {
            EXPECT_EQ(words[w], first[i][w]) << "chunk " << i << " word " << w;
        }
    }
    // the window past the end answers Done again, nothing else
    this->clearHistory();
    this->dump(static_cast<U16>(chunks), 8, Fw::CmdResponse::OK);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CoverageDumpDone_SIZE(1);
    ASSERT_EVENTS_CoverageDumpDone(0, objects, totalBytes, crc);
}

void RustTesterTester ::windowWithoutSnapshotRejected() {
    REQUIREMENT("REQ-RustTester-003");
    COMMENT("UT-RT-5: with a snapshot (0, 8) a window (1, 8) is served; count = 0 emits CoverageReset and drops it: "
            "(1, 8) is then an EXECUTION_ERROR with CoverageRejected(NO_SNAPSHOT)");
    if (!RustTester::coverageAvailable()) {
        GTEST_SKIP() << "built without FPRIME_RUST_REFERENCE_COVERAGE: no gcov information section";
    }
    // a snapshot exists: a later window is served
    this->clearHistory();
    this->dump(0, 8, Fw::CmdResponse::OK);
    this->clearHistory();
    this->dump(1, 8, Fw::CmdResponse::OK);
    ASSERT_EVENTS_CoverageRejected_SIZE(0);
    // zeroing drops it
    this->clearHistory();
    this->dump(0, 0, Fw::CmdResponse::OK);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CoverageReset_SIZE(1);
    this->clearHistory();
    this->dump(1, 8, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CoverageRejected_SIZE(1);
    ASSERT_EVENTS_CoverageRejected(0, RustTester_DumpReject::NO_SNAPSHOT);
}

void RustTesterTester ::zeroResetsCounters() {
    REQUIREMENT("REQ-RustTester-010");
    COMMENT("UT-RT-6: 50 MARKs then walk 1 (everything flushed so far), zero (counters and files), walk A, zero, walk "
            "B: A and B are shorter than walk 1 (zeroed counters are omitted from the stream) and differ from it; "
            "CoverageReset each time");
    if (!RustTester::coverageAvailable()) {
        GTEST_SKIP() << "built without FPRIME_RUST_REFERENCE_COVERAGE: no gcov information section";
    }
    // markers flush to disk: walk 1 sees everything this process accumulated (on disk + in memory)
    for (U32 i = 0; i < 50; i++) {
        this->sendCmd_MARK(0, 2000 + i, i);
    }
    U32 objects1 = 0, bytes1 = 0, crc1 = 0, chunksCrc1 = 0;
    (void)this->walkAll(objects1, bytes1, crc1, chunksCrc1);
    this->clearHistory();
    this->dump(0, 0, Fw::CmdResponse::OK);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CoverageReset(0, objects1);
    U32 objectsA = 0, bytesA = 0, crcA = 0, chunksCrcA = 0;
    (void)this->walkAll(objectsA, bytesA, crcA, chunksCrcA);
    this->clearHistory();
    this->dump(0, 0, Fw::CmdResponse::OK);
    ASSERT_EVENTS_CoverageReset_SIZE(1);
    U32 objectsB = 0, bytesB = 0, crcB = 0, chunksCrcB = 0;
    (void)this->walkAll(objectsB, bytesB, crcB, chunksCrcB);
    ASSERT_EQ(objects1, objectsA);
    ASSERT_EQ(objectsA, objectsB);
    // the zero empties the counters: every function this process ran before the first zero (all earlier tests)
    // disappears from the stream (GCC omits all-zero counter records), so A and B are shorter than walk 1 and
    // differ from it; A and B are not compared byte for byte because the instrumented tester itself runs
    // between a zero and the next snapshot
    ASSERT_LT(bytesA, bytes1);
    ASSERT_LT(bytesB, bytes1);
    ASSERT_NE(crc1, crcA);
    ASSERT_NE(crc1, crcB);
}

void RustTesterTester ::crc32KnownAnswer() {
    REQUIREMENT("REQ-RustTester-003");
    REQUIREMENT("REQ-RustTester-004");
    COMMENT("UT-RT-7: crc32 is CRC-32/IEEE: check value 0xcbf43926 over \"123456789\", continuation equals one pass");
    const U8 check[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    ASSERT_EQ(RustTester::crc32(0, check, 9), 0xCBF43926U);
    ASSERT_EQ(RustTester::crc32(RustTester::crc32(0, check, 4), &check[4], 5), 0xCBF43926U);
    ASSERT_EQ(RustTester::crc32(0, nullptr, 0), 0U);
}

void RustTesterTester ::injectedSnapshotWindows() {
    REQUIREMENT("REQ-RustTester-002");
    REQUIREMENT("REQ-RustTester-003");
    REQUIREMENT("REQ-RustTester-004");
    COMMENT("UT-RT-8: white-box, runs in every build: a 40-byte and a 32-byte object are paged as chunks "
            "(MSB-first words, zero padding, CoverageObject with each last chunk, Done only at the end)");
    U8* const snap = this->component.m_snapshot;
    for (U32 i = 0; i < 72; i++) {
        snap[i] = static_cast<U8>(i + 1);
    }
    this->component.m_objectCount = 2;
    this->component.m_objects[0].offset = 0;
    this->component.m_objects[0].bytes = 40;
    this->component.m_objects[0].nameCrc = 0x11111111U;
    this->component.m_objects[1].offset = 40;
    this->component.m_objects[1].bytes = 32;
    this->component.m_objects[1].nameCrc = 0x22222222U;
    this->component.m_snapshotBytes = 72;
    this->component.m_snapshotCrc = RustTester::crc32(0, snap, 72);
    this->component.m_snapshotValid = true;
    // white-box: emitWindow is driven directly (the OFF image answers CoverageUnavailable at the command)
    // window 0..1: object 0 chunk 0 (full) and chunk 1 (8 bytes, last)
    this->clearHistory();
    this->component.emitWindow(0, 2);
    ASSERT_EVENTS_CoverageChunk_SIZE(2);
    ASSERT_EVENTS_CoverageChunk(0, 0, 0, 32, 0x01020304U, 0x05060708U, 0x090A0B0CU, 0x0D0E0F10U, 0x11121314U,
                                0x15161718U, 0x191A1B1CU, 0x1D1E1F20U);
    ASSERT_EVENTS_CoverageChunk(1, 0, 32, 8, 0x21222324U, 0x25262728U, 0U, 0U, 0U, 0U, 0U, 0U);
    ASSERT_EVENTS_CoverageObject_SIZE(1);
    ASSERT_EVENTS_CoverageObject(0, 0, 0x11111111U, 40);
    ASSERT_EVENTS_CoverageDumpDone_SIZE(0);
    // window 2..9: object 1's single chunk (a multiple of 32: last chunk is full), then Done
    this->clearHistory();
    this->component.emitWindow(2, 8);
    ASSERT_EVENTS_CoverageChunk_SIZE(1);
    ASSERT_EVENTS_CoverageChunk(0, 1, 0, 32, 0x292A2B2CU, 0x2D2E2F30U, 0x31323334U, 0x35363738U, 0x393A3B3CU,
                                0x3D3E3F40U, 0x41424344U, 0x45464748U);
    ASSERT_EVENTS_CoverageObject_SIZE(1);
    ASSERT_EVENTS_CoverageObject(0, 1, 0x22222222U, 32);
    ASSERT_EVENTS_CoverageDumpDone_SIZE(1);
    ASSERT_EVENTS_CoverageDumpDone(0, 2, 72, this->component.m_snapshotCrc);
    // a window entirely past the end: Done only
    this->clearHistory();
    this->component.emitWindow(3, 8);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CoverageDumpDone_SIZE(1);
}

void RustTesterTester ::markFlushesAndResetUnlinks() {
    REQUIREMENT("REQ-RustTester-012");
    REQUIREMENT("REQ-RustTester-010");
    COMMENT("UT-RT-9: after a zero the first object's on-disk .gcda is gone; one MARK writes it back; a zero deletes "
            "it again (persistence point = marker; reset = counters + files)");
    if (!RustTester::coverageAvailable()) {
        GTEST_SKIP() << "built without FPRIME_RUST_REFERENCE_COVERAGE: no gcov information section";
    }
    char path[1024];
    ASSERT_TRUE(this->component.objectPath(0, path, sizeof path));
    ASSERT_NE(strstr(path, ".gcda"), nullptr) << path;
    FwSizeType size = 0;
    this->dump(0, 0, Fw::CmdResponse::OK);
    ASSERT_NE(Os::FileSystem::getFileSize(path, size), Os::FileSystem::OP_OK) << path << " should be gone";
    this->clearHistory();
    this->sendCmd_MARK(0, 1, 9);
    ASSERT_EVENTS_Marker_SIZE(1);
    ASSERT_EQ(Os::FileSystem::getFileSize(path, size), Os::FileSystem::OP_OK) << path << " should exist";
    ASSERT_GT(size, 0U);
    // a second marker merges into the same file (cumulative): it stays, at least as large
    this->sendCmd_MARK(0, 2, 10);
    FwSizeType size2 = 0;
    ASSERT_EQ(Os::FileSystem::getFileSize(path, size2), Os::FileSystem::OP_OK) << path << " should still exist";
    ASSERT_GE(size2, size);
    this->dump(0, 0, Fw::CmdResponse::OK);
    ASSERT_NE(Os::FileSystem::getFileSize(path, size), Os::FileSystem::OP_OK) << path << " should be gone";
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
            bool last = e.length < RustTester::CHUNK_BYTES;
            for (FwSizeType k = 0; !last && k < this->eventHistory_CoverageObject->size(); k++) {
                const EventEntry_CoverageObject& o = this->eventHistory_CoverageObject->at(k);
                last = (o.object == objectsSeen) && (o.bytes == objectBytes);
            }
            if (last) {
                objectsSeen++;
                objectBytes = 0;
            }
        }
        for (FwSizeType i = 0; i < this->eventHistory_CoverageObject->size(); i++) {
            const EventEntry_CoverageObject& o = this->eventHistory_CoverageObject->at(i);
            EXPECT_LT(o.object, objectsSeen);
            EXPECT_GT(o.bytes, 0U);
            EXPECT_NE(o.nameCrc, 0U);
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


// ----------------------------------------------------------------------
// BURST_CMD_U32 (UT-RT-10..12)
// ----------------------------------------------------------------------

void RustTesterTester ::burstCountBounds() {
    const FwOpcodeType burstOpcode = 0x10017000;  // rustMonitor.SET_LIMIT in the reference deployment
    this->sendCmd_BURST_CMD_U32(0, 1, 0, burstOpcode, 7);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, RustTester::OPCODE_BURST_CMD_U32, 1, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_from_cmdBurstOut_SIZE(0);

    this->sendCmd_BURST_CMD_U32(0, 2, 17, burstOpcode, 7);
    ASSERT_CMD_RESPONSE_SIZE(2);
    ASSERT_CMD_RESPONSE(1, RustTester::OPCODE_BURST_CMD_U32, 2, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_from_cmdBurstOut_SIZE(0);
    ASSERT_EVENTS_BurstSent_SIZE(0);

    this->sendCmd_BURST_CMD_U32(0, 3, 1, burstOpcode, 7);
    ASSERT_CMD_RESPONSE(2, RustTester::OPCODE_BURST_CMD_U32, 3, Fw::CmdResponse::OK);
    ASSERT_from_cmdBurstOut_SIZE(1);
    ASSERT_EVENTS_BurstSent_SIZE(1);
    ASSERT_EVENTS_BurstSent(0, 1, burstOpcode);

    this->clearHistory();
    this->sendCmd_BURST_CMD_U32(0, 4, 16, burstOpcode, 7);
    ASSERT_CMD_RESPONSE(0, RustTester::OPCODE_BURST_CMD_U32, 4, Fw::CmdResponse::OK);
    ASSERT_from_cmdBurstOut_SIZE(16);
    ASSERT_EVENTS_BurstSent_SIZE(1);
    ASSERT_EVENTS_BurstSent(0, 16, burstOpcode);
    for (U32 i = 0; i < 16; i++) {
        ASSERT_EQ(this->fromPortHistory_cmdBurstOut->at(i).context, i);
    }
}

void RustTesterTester ::burstPacketKnownAnswer() {
    const FwOpcodeType burstOpcode = 0x10017000;
    const U32 arg = 0x01020304;
    this->sendCmd_BURST_CMD_U32(0, 1, 2, burstOpcode, arg);
    ASSERT_from_cmdBurstOut_SIZE(2);

    // raw bytes: FW_PACKET_COMMAND descriptor (FwPacketDescriptorType), opcode (FwOpcodeType), U32 arg, big endian
    const U8 expected[] = {0x00, 0x00, 0x10, 0x01, 0x70, 0x00, 0x01, 0x02, 0x03, 0x04};
    static_assert(sizeof(FwPacketDescriptorType) == 2 && sizeof(FwOpcodeType) == 4, "KAT assumes U16 descriptor, U32 opcode");
    for (U32 copy = 0; copy < 2; copy++) {
        Fw::ComBuffer data = this->fromPortHistory_cmdBurstOut->at(copy).data;
        ASSERT_EQ(data.getSize(), sizeof(expected));
        for (U32 i = 0; i < sizeof(expected); i++) {
            ASSERT_EQ(data.getBuffAddr()[i], expected[i]) << "copy " << copy << " byte " << i;
        }
        // the dispatcher's own parser must accept it and see the same opcode and argument
        Fw::CmdPacket pkt;
        ASSERT_EQ(pkt.deserializeFrom(data), Fw::FW_SERIALIZE_OK);
        ASSERT_EQ(pkt.getOpCode(), burstOpcode);
        U32 seen = 0;
        ASSERT_EQ(pkt.getArgBuffer().deserializeTo(seen), Fw::FW_SERIALIZE_OK);
        ASSERT_EQ(seen, arg);
        ASSERT_EQ(pkt.getArgBuffer().getDeserializeSizeLeft(), 0u);
    }
}

void RustTesterTester ::burstResponseCounting() {
    const FwOpcodeType burstOpcode = 0x10017000;
    this->sendCmd_BURST_CMD_U32(0, 1, 12, burstOpcode, 7);
    ASSERT_from_cmdBurstOut_SIZE(12);
    // the burst zeroes both channels first
    ASSERT_TLM_BurstResponsesOk_SIZE(1);
    ASSERT_TLM_BurstResponsesOk(0, 0);
    ASSERT_TLM_BurstResponsesError_SIZE(1);
    ASSERT_TLM_BurstResponsesError(0, 0);

    for (U32 i = 0; i < 10; i++) {
        this->invoke_to_cmdBurstStatus(0, burstOpcode, i, Fw::CmdResponse(Fw::CmdResponse::OK));
    }
    this->invoke_to_cmdBurstStatus(0, burstOpcode, 10, Fw::CmdResponse(Fw::CmdResponse::BUSY));
    this->invoke_to_cmdBurstStatus(0, burstOpcode, 11, Fw::CmdResponse(Fw::CmdResponse::EXECUTION_ERROR));
    ASSERT_TLM_BurstResponsesOk_SIZE(11);
    ASSERT_TLM_BurstResponsesOk(10, 10);
    ASSERT_TLM_BurstResponsesError_SIZE(3);
    ASSERT_TLM_BurstResponsesError(2, 2);

    // a new burst starts both counts over
    this->clearHistory();
    this->sendCmd_BURST_CMD_U32(0, 2, 1, burstOpcode, 7);
    ASSERT_TLM_BurstResponsesOk(0, 0);
    ASSERT_TLM_BurstResponsesError(0, 0);
    this->invoke_to_cmdBurstStatus(0, burstOpcode, 0, Fw::CmdResponse(Fw::CmdResponse::OK));
    ASSERT_TLM_BurstResponsesOk(1, 1);
}
}  // namespace FprimeRustReference
