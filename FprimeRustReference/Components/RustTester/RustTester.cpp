// ======================================================================
// \title  RustTester.cpp
// \author lestarch-autobot
// \brief  cpp file for RustTester component implementation class
// ======================================================================

#include "FprimeRustReference/Components/RustTester/RustTester.hpp"

#include <cstring>

#include "Fw/Types/Assert.hpp"

// libgcov interface (GCC >= 12 with -fprofile-info-section=gcov_info), declared without <gcov.h>. In the
// instrumented build (FPRIME_RUST_REFERENCE_COVERAGE=1 from CMake) the references are strong so the linker
// pulls them out of libgcov.a; in the pure (OFF) image they are weak, resolve to null on any GCC and the
// component reports CoverageUnavailable. GNU ld provides __start_/__stop_gcov_info for the C-identifier
// section name whenever the section exists.
#if defined(FPRIME_RUST_REFERENCE_COVERAGE) && FPRIME_RUST_REFERENCE_COVERAGE
#define RUST_TESTER_LIBGCOV
#else
#define RUST_TESTER_LIBGCOV __attribute__((weak))
#endif
struct gcov_info;
extern "C" {
extern const struct gcov_info* const __start_gcov_info[] __attribute__((weak));
extern const struct gcov_info* const __stop_gcov_info[] __attribute__((weak));
extern void __gcov_info_to_gcda(const struct gcov_info* info,
                                void (*filename_fn)(const char*, void*),
                                void (*dump_fn)(const void*, unsigned, void*),
                                void* (*allocate_fn)(unsigned, void*),
                                void* arg) RUST_TESTER_LIBGCOV;
extern void __gcov_init(struct gcov_info* info) RUST_TESTER_LIBGCOV;
extern void __gcov_reset(void) RUST_TESTER_LIBGCOV;
}

// Everything that runs between two dump windows must not move the counters it describes (harness §3a)
#define RUST_TESTER_NO_PROFILE __attribute__((no_profile_instrument_function))

namespace FprimeRustReference {

// out-of-class definitions: the constants are odr-used (passed by reference in the unit tests)
const U8 RustTester::MAX_CHUNKS_PER_WINDOW;
const U32 RustTester::CHUNK_BYTES;
const U32 RustTester::SNAPSHOT_BYTES;
const U32 RustTester::MAX_OBJECTS;
const U32 RustTester::ARENA_BYTES;

// libgcov keeps one process-wide list: register the section's objects exactly once per process, whatever the
// number of component instances (a second registration would make the list circular and __gcov_reset loop)
bool RustTester::s_registered = false;

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

RustTester ::RustTester(const char* const compName)
    : RustTesterComponentBase(compName),
      m_objectCount(0),
      m_snapshotBytes(0),
      m_snapshotCrc(0),
      m_snapshotValid(false),
      m_snapshotOverflow(false),
      m_arenaUsed(0) {}

RustTester ::~RustTester() {}

RUST_TESTER_NO_PROFILE bool RustTester ::coverageAvailable() {
    return (__start_gcov_info != nullptr) && (__stop_gcov_info != nullptr) && (__gcov_info_to_gcda != nullptr) &&
           (__start_gcov_info != __stop_gcov_info);
}

RUST_TESTER_NO_PROFILE U32 RustTester ::crc32(U32 crc, const U8* data, FwSizeType length) {
    FW_ASSERT(data != nullptr || length == 0);
    crc = ~crc;
    for (FwSizeType i = 0; i < length; i++) {
        crc ^= data[i];
        for (U32 bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
        }
    }
    return ~crc;
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

RUST_TESTER_NO_PROFILE void RustTester ::MARK_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 id) {
    this->log_ACTIVITY_LO_Marker(id);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

RUST_TESTER_NO_PROFILE void RustTester ::DUMP_COVERAGE_cmdHandler(FwOpcodeType opCode,
                                                                   U32 cmdSeq,
                                                                   U16 start,
                                                                   U8 count) {
    if (count > MAX_CHUNKS_PER_WINDOW) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    if (!RustTester::coverageAvailable()) {
        this->log_WARNING_LO_CoverageUnavailable();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
        return;
    }
    if (count == 0) {
        this->zeroCounters();
        this->m_snapshotValid = false;
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
        return;
    }
    if (start == 0) {
        if (!this->takeSnapshot()) {
            this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }
    } else if (!this->m_snapshotValid) {
        // a window that does not start the walk needs the snapshot a chunk-0 window took
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    this->emitWindow(static_cast<U32>(start), static_cast<U32>(count));
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Coverage dump path
// ----------------------------------------------------------------------

RUST_TESTER_NO_PROFILE bool RustTester ::takeSnapshot() {
    this->m_objectCount = 0;
    this->m_snapshotBytes = 0;
    this->m_snapshotOverflow = false;
    this->m_snapshotValid = false;
    for (const struct gcov_info* const* it = __start_gcov_info; it != __stop_gcov_info; ++it) {
        if (this->m_objectCount >= MAX_OBJECTS) {
            return false;
        }
        ObjectRecord& record = this->m_objects[this->m_objectCount];
        record.offset = this->m_snapshotBytes;
        record.bytes = 0;
        record.nameCrc = 0;
        this->m_arenaUsed = 0;
        __gcov_info_to_gcda(*it, &RustTester::filenameCallback, &RustTester::dumpCallback,
                            &RustTester::allocateCallback, this);
        if (this->m_snapshotOverflow) {
            return false;
        }
        record.bytes = this->m_snapshotBytes - record.offset;
        this->m_objectCount++;
    }
    this->m_snapshotCrc = RustTester::crc32(0, this->m_snapshot, this->m_snapshotBytes);
    this->m_snapshotValid = true;
    return true;
}

RUST_TESTER_NO_PROFILE void RustTester ::emitWindow(U32 first, U32 count) {
    FW_ASSERT(this->m_snapshotValid);
    const U32 end = first + count;
    U32 chunkIndex = 0;
    for (U32 object = 0; object < this->m_objectCount; object++) {
        const ObjectRecord& record = this->m_objects[object];
        for (U32 offset = 0; offset < record.bytes; offset += CHUNK_BYTES) {
            const U32 remaining = record.bytes - offset;
            const U32 length = (remaining < CHUNK_BYTES) ? remaining : CHUNK_BYTES;
            if (chunkIndex >= first && chunkIndex < end) {
                U32 words[CHUNK_BYTES / 4] = {0, 0, 0, 0, 0, 0, 0, 0};
                const U8* const bytes = &this->m_snapshot[record.offset + offset];
                for (U32 i = 0; i < length; i++) {
                    words[i / 4] |= static_cast<U32>(bytes[i]) << (24 - 8 * (i % 4));
                }
                this->log_ACTIVITY_LO_CoverageChunk(object, offset, length, words[0], words[1], words[2],
                                                    words[3], words[4], words[5], words[6], words[7]);
                if (offset + length == record.bytes) {
                    this->log_ACTIVITY_LO_CoverageObject(object, record.nameCrc, record.bytes);
                }
            }
            chunkIndex++;
        }
    }
    if (end >= chunkIndex) {
        this->log_ACTIVITY_LO_CoverageDumpDone(this->m_objectCount, this->m_snapshotBytes, this->m_snapshotCrc);
    }
}

RUST_TESTER_NO_PROFILE void RustTester ::zeroCounters() {
    if (__gcov_init == nullptr || __gcov_reset == nullptr) {
        return;
    }
    if (!RustTester::s_registered) {
        // -fprofile-info-section skips libgcov's constructor registration; register once so __gcov_reset
        // reaches every object. Side effect: libgcov's exit destructor then also writes these objects' .gcda
        // files at process exit (only in images that ever received a count = 0); the harness overwrites them.
        for (const struct gcov_info* const* it = __start_gcov_info; it != __stop_gcov_info; ++it) {
            __gcov_init(const_cast<struct gcov_info*>(*it));
        }
        RustTester::s_registered = true;
    }
    __gcov_reset();
}

RUST_TESTER_NO_PROFILE void RustTester ::filenameCallback(const char* filename, void* arg) {
    FW_ASSERT(arg != nullptr);
    RustTester* const self = static_cast<RustTester*>(arg);
    FW_ASSERT(self->m_objectCount < MAX_OBJECTS, static_cast<FwAssertArgType>(self->m_objectCount));
    if (filename != nullptr) {
        self->m_objects[self->m_objectCount].nameCrc =
            RustTester::crc32(0, reinterpret_cast<const U8*>(filename), static_cast<FwSizeType>(strlen(filename)));
    }
}

RUST_TESTER_NO_PROFILE void RustTester ::dumpCallback(const void* data, unsigned length, void* arg) {
    FW_ASSERT(arg != nullptr);
    RustTester* const self = static_cast<RustTester*>(arg);
    if (self->m_snapshotOverflow || (static_cast<U32>(length) > SNAPSHOT_BYTES - self->m_snapshotBytes)) {
        self->m_snapshotOverflow = true;
        return;
    }
    (void)memcpy(&self->m_snapshot[self->m_snapshotBytes], data, length);
    self->m_snapshotBytes += static_cast<U32>(length);
}

RUST_TESTER_NO_PROFILE void* RustTester ::allocateCallback(unsigned length, void* arg) {
    FW_ASSERT(arg != nullptr);
    RustTester* const self = static_cast<RustTester*>(arg);
    // 8-byte aligned bump allocation from the static arena; libgcov only asks for top-n counter scratch
    const U32 aligned = (static_cast<U32>(length) + 7U) & ~7U;
    FW_ASSERT(aligned <= ARENA_BYTES - self->m_arenaUsed, static_cast<FwAssertArgType>(length));
    void* const block = &self->m_arena[self->m_arenaUsed];
    self->m_arenaUsed += aligned;
    return block;
}

}  // namespace FprimeRustReference
