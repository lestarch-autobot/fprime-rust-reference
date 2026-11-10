// ======================================================================
// \title  RustTester.hpp
// \author lestarch-autobot
// \brief  hpp file for RustTester component implementation class
// ======================================================================

#ifndef FprimeRustReference_RustTester_HPP
#define FprimeRustReference_RustTester_HPP

#include "FprimeRustReference/Components/RustTester/RustTesterComponentAc.hpp"

namespace FprimeRustReference {

//! Harness tester: run-start markers and gcov coverage dumps (fprime-system-test-harness §1(4), §3a)
//!
//! DUMP_COVERAGE(start, count) pages the gcda stream of the instrumented twin image in global 32-byte
//! chunks. The stream is snapshotted when a window starts at chunk 0 (the deployment keeps running
//! between paced windows, so a live walk would describe different counter states); later windows read
//! the snapshot. count 0 zeroes the counters. Without instrumentation only CoverageUnavailable is emitted.
class RustTester final : public RustTesterComponentBase {
  public:
    //! Largest window: 8 chunks + CoverageObject + CoverageDumpDone fit the 20-deep CdhCore.cmdDisp queue
    static const U8 MAX_CHUNKS_PER_WINDOW = 8;   //!< events per window stay far below CdhCoreConfig.QueueSizes.events
    static const U32 CHUNK_BYTES = 32;           //!< CoverageChunk payload (8 x U32)
    //! Capacity of the snapshot storage (gcda bytes of all instrumented objects); sized in RustTester.cpp by the
    //! build (twin: 512 KiB, OFF image: 1 KiB), never in this header so every translation unit sees one layout
    static U32 snapshotCapacity();
    //! Capacity of the object table (twin: 64, OFF: 4)
    static U32 objectCapacity();

    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct RustTester object
    RustTester(const char* const compName  //!< The component name
    );

    //! Destroy RustTester object
    ~RustTester();

    //! Initialize the component. In the coverage twin the libgcov registration and the exit-time flush are done by
    //! a file-scope guard constructed before every default-priority static (so the flush runs after the static
    //! destructors); init() repeats the idempotent registration for completeness. Hides the non-virtual autocoded
    //! init: call it on the derived type (the topology and the unit tests do)
    void init(FwEnumStoreType instance = 0);

    //! Whether the image carries a gcov information section (built with FPRIME_RUST_REFERENCE_COVERAGE)
    static bool coverageAvailable();

    //! CRC-32/IEEE (zlib compatible) continuation over `length` bytes
    static U32 crc32(U32 crc, const U8* data, FwSizeType length);

    //! One snapshotted object: its gcda bytes live at m_snapshot[offset .. offset + bytes)
    struct ObjectRecord {
        U32 offset;   //!< First byte in m_snapshot
        U32 bytes;    //!< gcda bytes
        U32 nameCrc;  //!< CRC-32 of the compiler's .gcda path
    };

  private:
    friend class RustTesterTester;
    friend struct RustTesterCoverageGuard;  //!< init_priority(101) registration guard in RustTester.cpp (twin)  //!< white-box unit tests inject a snapshot (UT-RT-7/8) and read object paths (UT-RT-9)

    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command MARK
    //!
    //! Run-start marker: emits Marker(id) synchronously, then responds OK
    void MARK_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                         U32 cmdSeq,           //!< The command sequence number
                         U32 id                //!< Marker id echoed in the event
                         ) override;

    //! Handler implementation for command DUMP_COVERAGE
    //!
    //! Coverage window: emits the global 32-byte gcda chunks start..start+count-1 (count 1..8);
    //! count 0 zeroes the counters. Without instrumentation only CoverageUnavailable is emitted.
    void DUMP_COVERAGE_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                  U32 cmdSeq,           //!< The command sequence number
                                  U16 start,            //!< First global chunk index of the window
                                  U8 count  //!< Chunks in the window (0 = zero the counters; > 8 is a
                                            //!< validation error)
                                  ) override;

    // ----------------------------------------------------------------------
    // Coverage dump path (never instrumented, see RustTester.cpp)
    // ----------------------------------------------------------------------

    //! Flush the counters and read every object's on-disk .gcda into m_snapshot; false when a file does not fit
    //! (m_snapshotOverflow) or cannot be read (m_fileError)
    bool takeSnapshot();

    //! Emit the window [first, first + count) from the snapshot
    void emitWindow(U32 first, U32 count);

    //! What filenameCallback does with the .gcda path libgcov names for the current object
    enum FileMode {
        FILE_READ,    //!< append the file's bytes to the snapshot (takeSnapshot)
        FILE_UNLINK,  //!< delete the file (zeroCounters)
        FILE_PATH     //!< copy the path into m_pathOut (unit tests)
    };

    //! Register the section's objects with libgcov once per process and install the exit flush (idempotent; no-op
    //! without instrumentation). Called by the init_priority(101) guard in the twin and again by init()
    static void registerObjects();

    //! Flush the in-memory counters into the on-disk .gcda files and zero them (libgcov dump + reset)
    static void flushCounters();

    //! atexit hook of the twin: flush the tail after the last marker
    static void flushAtExit();

    //! Zero the arc counters of every object and delete their on-disk .gcda files
    void zeroCounters();

    //! Walk the section, calling filenameCallback in `mode` for every object (FILE_READ sets m_snapshotOverflow /
    //! m_fileError when a file does not fit or cannot be read)
    void forEachObjectFile(FileMode mode);

    //! Path of the .gcda file of object `index` (unit tests); false when there is no such object
    bool objectPath(U32 index, char* buffer, FwSizeType capacity);

    //! libgcov callbacks (arg is the RustTester)
    static void filenameCallback(const char* filename, void* arg);
    static void dumpCallback(const void* data, unsigned length, void* arg);
    static void* allocateCallback(unsigned length, void* arg);

    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    U8* const m_snapshot;                   //!< gcda bytes of all objects, in walk order (file-scope storage)
    ObjectRecord* const m_objects;          //!< per-object table (file-scope storage)
    U32 m_objectCount;                      //!< objects in the snapshot
    U32 m_snapshotBytes;                    //!< bytes used in m_snapshot
    U32 m_snapshotCrc;                      //!< CRC-32 over m_snapshot[0 .. m_snapshotBytes)
    bool m_snapshotValid;                   //!< a window at chunk 0 has been taken since the last reset
    bool m_snapshotOverflow;                //!< the walk did not fit (set by filenameCallback / forEachObjectFile)
    U8* const m_arena;                      //!< allocateCallback scratch (8-byte aligned file-scope storage), reset per object
    U32 m_arenaUsed;                        //!< bytes handed out of m_arena for the current object
    FileMode m_fileMode;                    //!< what filenameCallback does for the object being walked
    bool m_fileError;                       //!< a .gcda file could not be read (set by filenameCallback)
    U32 m_pathIndex;                        //!< FILE_PATH: object whose path is wanted
    U32 m_walkIndex;                        //!< object index during a forEachObjectFile walk
    char* m_pathOut;                        //!< FILE_PATH: destination buffer
    FwSizeType m_pathCapacity;              //!< FILE_PATH: destination capacity
    bool m_pathFound;                       //!< FILE_PATH: the path was copied
    static bool s_registered;               //!< objects registered with libgcov for __gcov_reset (process-wide)
    static U32 s_instances;                 //!< live instances: the storage is single-instance
};

}  // namespace FprimeRustReference

#endif
