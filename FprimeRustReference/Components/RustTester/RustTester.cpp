// ======================================================================
// \title  RustTester.cpp
// \author lestarch-autobot
// \brief  cpp file for RustTester component implementation class
// ======================================================================

#include "FprimeRustReference/Components/RustTester/RustTester.hpp"

#include <cstdlib>
#include <cstring>

#include "Fw/Types/Assert.hpp"
#include "Os/File.hpp"
#include "Os/FileSystem.hpp"

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
// the entries point at writable libgcov records (__gcov_init links them through info->next)
extern struct gcov_info* const __start_gcov_info[] __attribute__((weak));
extern struct gcov_info* const __stop_gcov_info[] __attribute__((weak));
extern void __gcov_info_to_gcda(const struct gcov_info* info,
                                void (*filename_fn)(const char*, void*),
                                void (*dump_fn)(const void*, unsigned, void*),
                                void* (*allocate_fn)(unsigned, void*),
                                void* arg) RUST_TESTER_LIBGCOV;
extern void __gcov_init(struct gcov_info* info) RUST_TESTER_LIBGCOV;
extern void __gcov_reset(void) RUST_TESTER_LIBGCOV;
extern void __gcov_dump(void) RUST_TESTER_LIBGCOV;
}

// Everything that runs between two dump windows must not move the counters it describes (harness §3a)
#define RUST_TESTER_NO_PROFILE __attribute__((no_profile_instrument_function))

namespace FprimeRustReference {

// out-of-class definitions: the constants are odr-used (passed by reference in the unit tests)
const U8 RustTester::MAX_CHUNKS_PER_WINDOW;
const U32 RustTester::CHUNK_BYTES;

// Snapshot storage, sized by the build and owned by this translation unit so the class layout does not depend on
// FPRIME_RUST_REFERENCE_COVERAGE (the topology instantiates RustTester from a translation unit without it).
// One instance per process (asserted in the constructor); the OFF image spends about 1 KiB.
namespace {
#if defined(FPRIME_RUST_REFERENCE_COVERAGE) && FPRIME_RUST_REFERENCE_COVERAGE
const U32 SNAPSHOT_BYTES = 512 * 1024;  // gcda bytes of all instrumented objects (twin)
const U32 MAX_OBJECTS = 64;             // instrumented translation units (twin)
const U32 ARENA_BYTES = 16 * 1024;      // libgcov allocate callback scratch (twin)
#else
const U32 SNAPSHOT_BYTES = 1024;  // enough for the white-box unit test
const U32 MAX_OBJECTS = 4;
const U32 ARENA_BYTES = 64;
#endif
U8 s_snapshot[SNAPSHOT_BYTES];
RustTester::ObjectRecord s_objects[MAX_OBJECTS];
alignas(8) U8 s_arena[ARENA_BYTES];
}  // namespace

U32 RustTester::s_instances = 0;

// libgcov keeps one process-wide list: register the section's objects exactly once per process, whatever the
// number of component instances (a second registration would make the list circular and __gcov_reset loop)
bool RustTester::s_registered = false;

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

RUST_TESTER_NO_PROFILE RustTester ::RustTester(const char* const compName)
    : RustTesterComponentBase(compName),
      m_snapshot(s_snapshot),
      m_objects(s_objects),
      m_objectCount(0),
      m_snapshotBytes(0),
      m_snapshotCrc(0),
      m_snapshotValid(false),
      m_snapshotOverflow(false),
      m_arena(s_arena),
      m_arenaUsed(0),
      m_fileMode(FILE_READ),
      m_fileError(false),
      m_pathIndex(0),
      m_walkIndex(0),
      m_pathOut(nullptr),
      m_pathCapacity(0),
      m_pathFound(false) {
    RustTester::s_instances++;
    FW_ASSERT(RustTester::s_instances == 1, static_cast<FwAssertArgType>(RustTester::s_instances));
}

RUST_TESTER_NO_PROFILE RustTester ::~RustTester() {
    RustTester::s_instances--;
}

RUST_TESTER_NO_PROFILE void RustTester ::init(FwEnumStoreType instance) {
    RustTesterComponentBase::init(instance);
    RustTester::registerObjects();
}

RUST_TESTER_NO_PROFILE U32 RustTester ::snapshotCapacity() {
    return SNAPSHOT_BYTES;
}

RUST_TESTER_NO_PROFILE U32 RustTester ::objectCapacity() {
    return MAX_OBJECTS;
}

RUST_TESTER_NO_PROFILE bool RustTester ::coverageAvailable() {
#if defined(FPRIME_RUST_REFERENCE_COVERAGE) && FPRIME_RUST_REFERENCE_COVERAGE
    // strong libgcov references: only the section can be empty
    return (__start_gcov_info != nullptr) && (__start_gcov_info != __stop_gcov_info);
#else
    return (__start_gcov_info != nullptr) && (__stop_gcov_info != nullptr) && (__gcov_info_to_gcda != nullptr) &&
           (__start_gcov_info != __stop_gcov_info);
#endif
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
    // twin: the marker is also the persistence point — counters so far go to the on-disk .gcda files
    // (cumulative merge) and the in-memory counters restart at zero, so nothing is counted twice
    RustTester::flushCounters();
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
        this->log_ACTIVITY_LO_CoverageReset(
            static_cast<U32>(__stop_gcov_info - __start_gcov_info));
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
        return;
    }
    if (start == 0) {
        if (!this->takeSnapshot()) {
            this->log_WARNING_LO_CoverageRejected(this->m_fileError ? RustTester_DumpReject::FILE_ERROR
                                                                    : RustTester_DumpReject::OVERFLOW);
            this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }
    } else if (!this->m_snapshotValid) {
        // a window that does not start the walk needs the snapshot a chunk-0 window took
        this->log_WARNING_LO_CoverageRejected(RustTester_DumpReject::NO_SNAPSHOT);
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
    this->m_fileError = false;
    this->m_snapshotValid = false;
    // the on-disk files are the persistent, cumulative record (markers and earlier processes flushed into
    // them); bring them up to date, then the snapshot is their content
    RustTester::flushCounters();
    this->forEachObjectFile(FILE_READ);
    if (this->m_snapshotOverflow || this->m_fileError) {
        return false;
    }
    this->m_snapshotCrc = RustTester::crc32(0, this->m_snapshot, this->m_snapshotBytes);
    this->m_snapshotValid = true;
    return true;
}

RUST_TESTER_NO_PROFILE void RustTester ::forEachObjectFile(FileMode mode) {
    this->m_fileMode = mode;
    this->m_walkIndex = 0;
    for (struct gcov_info* const* it = __start_gcov_info; it != __stop_gcov_info; ++it) {
        if (mode == FILE_READ) {
            if (this->m_objectCount >= MAX_OBJECTS) {
                this->m_snapshotOverflow = true;
                return;
            }
            ObjectRecord& record = this->m_objects[this->m_objectCount];
            record.offset = this->m_snapshotBytes;
            record.bytes = 0;
            record.nameCrc = 0;
        }
        this->m_arenaUsed = 0;
        // libgcov names the object's .gcda path through the filename callback (the data callback is not used:
        // the bytes come from the file); this keeps the component independent of the gcov_info layout
        __gcov_info_to_gcda(*it, &RustTester::filenameCallback, &RustTester::dumpCallback,
                            &RustTester::allocateCallback, this);
        if (mode == FILE_READ) {
            if (this->m_snapshotOverflow || this->m_fileError) {
                return;
            }
            ObjectRecord& record = this->m_objects[this->m_objectCount];
            record.bytes = this->m_snapshotBytes - record.offset;
            this->m_objectCount++;
        }
        this->m_walkIndex++;
    }
}

RUST_TESTER_NO_PROFILE bool RustTester ::objectPath(U32 index, char* buffer, FwSizeType capacity) {
    FW_ASSERT(buffer != nullptr);
    if (!RustTester::coverageAvailable()) {
        return false;
    }
    this->m_pathIndex = index;
    this->m_pathOut = buffer;
    this->m_pathCapacity = capacity;
    this->m_pathFound = false;
    this->forEachObjectFile(FILE_PATH);
    this->m_pathOut = nullptr;
    return this->m_pathFound;
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

RUST_TESTER_NO_PROFILE void RustTester ::registerObjects() {
    if (__gcov_init == nullptr || !RustTester::coverageAvailable() || RustTester::s_registered) {
        return;
    }
    // -fprofile-info-section skips libgcov's constructor registration; register the section once per process so
    // __gcov_dump/__gcov_reset reach every object, and flush the tail at exit (the twin emits no exit handler
    // of its own). Only objects compiled through fprime_rust_reference_instrument (info section, no
    // constructor) may be in this image: a translation unit instrumented the ordinary way registers itself and
    // a second registration here would make libgcov's list circular.
    for (struct gcov_info* const* it = __start_gcov_info; it != __stop_gcov_info; ++it) {
        __gcov_init(*it);
    }
    RustTester::s_registered = true;
    (void)std::atexit(&RustTester::flushAtExit);
}

RUST_TESTER_NO_PROFILE void RustTester ::flushCounters() {
    if (__gcov_dump == nullptr || __gcov_reset == nullptr || !RustTester::s_registered) {
        return;
    }
    __gcov_dump();   // merge the in-memory counters into the on-disk .gcda of every registered object
    __gcov_reset();  // and start again from zero, so the files stay cumulative without double counting
}

RUST_TESTER_NO_PROFILE void RustTester ::flushAtExit() {
    RustTester::flushCounters();
}

RUST_TESTER_NO_PROFILE void RustTester ::zeroCounters() {
    if (__gcov_reset == nullptr) {
        return;
    }
    RustTester::registerObjects();
    __gcov_reset();
    this->forEachObjectFile(FILE_UNLINK);
}

RUST_TESTER_NO_PROFILE void RustTester ::filenameCallback(const char* filename, void* arg) {
    FW_ASSERT(arg != nullptr);
    RustTester* const self = static_cast<RustTester*>(arg);
    if (filename == nullptr) {
        self->m_fileError = (self->m_fileMode == FILE_READ);
        return;
    }
    switch (self->m_fileMode) {
        case FILE_READ: {
            FW_ASSERT(self->m_objectCount < MAX_OBJECTS, static_cast<FwAssertArgType>(self->m_objectCount));
            ObjectRecord& record = self->m_objects[self->m_objectCount];
            record.nameCrc = RustTester::crc32(0, reinterpret_cast<const U8*>(filename),
                                               static_cast<FwSizeType>(strlen(filename)));
            Os::File file;
            if (file.open(filename, Os::File::OPEN_READ) != Os::File::OP_OK) {
                self->m_fileError = true;
                return;
            }
            FwSizeType size = 0;
            if (file.size(size) != Os::File::OP_OK) {
                self->m_fileError = true;
                return;
            }
            if (size > static_cast<FwSizeType>(SNAPSHOT_BYTES - self->m_snapshotBytes)) {
                self->m_snapshotOverflow = true;
                return;
            }
            FwSizeType read = size;
            if (size > 0 && (file.read(&self->m_snapshot[self->m_snapshotBytes], read, Os::File::WAIT) !=
                                 Os::File::OP_OK ||
                             read != size)) {
                self->m_fileError = true;
                return;
            }
            self->m_snapshotBytes += static_cast<U32>(size);
            break;
        }
        case FILE_UNLINK:
            // a file that does not exist yet (never flushed) is already in the wanted state
            (void)Os::FileSystem::removeFile(filename);
            break;
        case FILE_PATH:
            if (self->m_walkIndex == self->m_pathIndex && self->m_pathOut != nullptr) {
                const FwSizeType length = static_cast<FwSizeType>(strlen(filename));
                if (length < self->m_pathCapacity) {
                    (void)memcpy(self->m_pathOut, filename, length + 1);
                    self->m_pathFound = true;
                }
            }
            break;
        default:
            FW_ASSERT(0, static_cast<FwAssertArgType>(self->m_fileMode));
            break;
    }
}

RUST_TESTER_NO_PROFILE void RustTester ::dumpCallback(const void* data, unsigned length, void* arg) {
    // the snapshot is read from the on-disk files; libgcov's in-memory serialization is not used
    (void)data;
    (void)length;
    FW_ASSERT(arg != nullptr);
}

RUST_TESTER_NO_PROFILE void* RustTester ::allocateCallback(unsigned length, void* arg) {
    FW_ASSERT(arg != nullptr);
    RustTester* const self = static_cast<RustTester*>(arg);
    // 8-byte aligned bump allocation from the arena (alignas(8) member, reset per object). libgcov asks only
    // for top-n counter scratch, which -fprofile-arcs -ftest-coverage never emit, and caches the one block it
    // gets in a static, so a single request per object is the invariant; bound before rounding (no wrap).
    const U32 free = ARENA_BYTES - self->m_arenaUsed;
    FW_ASSERT(static_cast<U32>(length) <= free, static_cast<FwAssertArgType>(length));
    const U32 aligned = (static_cast<U32>(length) + 7U) & ~7U;
    void* const block = &self->m_arena[self->m_arenaUsed];
    self->m_arenaUsed += (aligned <= free) ? aligned : free;
    return block;
}

}  // namespace FprimeRustReference
