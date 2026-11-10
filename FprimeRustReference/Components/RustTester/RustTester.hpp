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
    friend class RustTesterTester;  //!< white-box unit tests inject a snapshot (UT-RT-7/8)

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

    //! Serialize every object of the gcov information section into m_snapshot; false when it does not fit
    bool takeSnapshot();

    //! Emit the window [first, first + count) from the snapshot
    void emitWindow(U32 first, U32 count);

    //! Zero the arc counters of every object (registers them with libgcov once)
    void zeroCounters();

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
    bool m_snapshotOverflow;                //!< the walk did not fit (set by dumpCallback)
    U8* const m_arena;                      //!< allocateCallback scratch (8-byte aligned file-scope storage), reset per object
    U32 m_arenaUsed;                        //!< bytes handed out of m_arena for the current object
    static bool s_registered;               //!< objects registered with libgcov for __gcov_reset (process-wide)
    static U32 s_instances;                 //!< live instances: the storage is single-instance
};

}  // namespace FprimeRustReference

#endif
