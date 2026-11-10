module FprimeRustReference {

  @ Harness tester component (fprime-system-test-harness §1(4)): run-start markers for the auditor and
  @ gcov coverage dumps of the instrumented twin image. Plain C++, no Rust; harness, not UUT.
  passive component RustTester {

    # ----------------------------------------------------------------------
    # Types
    # ----------------------------------------------------------------------

    @ Why a DUMP_COVERAGE window was rejected
    enum DumpReject {
      NO_SNAPSHOT @< start > 0 but no chunk-0 window took a snapshot since the last reset
      OVERFLOW @< the gcda stream does not fit the snapshot buffer or the object table
      FILE_ERROR @< an on-disk .gcda file named by libgcov could not be read
    }

    # ----------------------------------------------------------------------
    # Special ports
    # ----------------------------------------------------------------------

    @ Time stamping for events
    time get port timeCaller

    @ Standard command interface
    import Fw.Command

    @ Standard event interface
    import Fw.Event

    # ----------------------------------------------------------------------
    # Commands
    # ----------------------------------------------------------------------

    @ Run-start marker: emits Marker(id) synchronously, then responds OK. In the coverage twin the handler also
    @ flushes the counters to the on-disk .gcda files (libgcov dump + reset), so coverage survives the process.
    sync command MARK(
      $id: U32 @< Marker id echoed in the event
    ) opcode 0

    @ Coverage window: emits the global 32-byte gcda chunks start..start+count-1 (count 1..8) of the on-disk
    @ .gcda files (flushed first); count 0 zeroes the counters and deletes those files. Without instrumentation
    @ only CoverageUnavailable is emitted.
    sync command DUMP_COVERAGE(
      start: U16 @< First global chunk index of the window
      count: U8 @< Chunks in the window (0 = zero the counters; > 8 is a validation error)
    ) opcode 1

    # ----------------------------------------------------------------------
    # Events
    # ----------------------------------------------------------------------

    @ Run-start marker (fth auditor: event name ends in `.Marker`, U32 `id`)
    event Marker(
      $id: U32 @< Marker id
    ) severity activity low id 0 format "Marker {}"

    @ One 32-byte gcda chunk (fth system_coverage contract text; data MSB-first, zero padded)
    event CoverageChunk(
      object: U32 @< Object index in walk order
      offset: U32 @< Byte offset of the chunk inside the object's gcda
      length: U32 @< Valid bytes in the chunk (1..32)
      data0: U32 @< Bytes 0..3
      data1: U32 @< Bytes 4..7
      data2: U32 @< Bytes 8..11
      data3: U32 @< Bytes 12..15
      data4: U32 @< Bytes 16..19
      data5: U32 @< Bytes 20..23
      data6: U32 @< Bytes 24..27
      data7: U32 @< Bytes 28..31
    ) severity activity low id 1 format "coverage chunk obj {} off {} len {}: {x} {x} {x} {x} {x} {x} {x} {x}"

    @ Emitted with an object's last chunk; nameCrc is the CRC-32/IEEE of the compiler's .gcda path
    event CoverageObject(
      object: U32 @< Object index in walk order
      nameCrc: U32 @< CRC-32 of the .gcda path string
      bytes: U32 @< gcda bytes of the object
    ) severity activity low id 2 format "coverage object {} name-crc {x}: {} bytes"

    @ Emitted when the window reaches the end of the walk; crc is the CRC-32 over all gcda bytes in object order
    event CoverageDumpDone(
      objects: U32 @< Objects walked
      totalBytes: U32 @< gcda bytes over all objects
      crc: U32 @< CRC-32 over all gcda bytes
    ) severity activity low id 3 format "coverage dump done: {} objects, {} bytes, crc {x}"

    @ The image was built without FPRIME_RUST_REFERENCE_COVERAGE: no gcov information section
    event CoverageUnavailable severity warning low id 4 format "coverage not compiled into this image"

    @ DUMP_COVERAGE(start, 0): the arc counters of every instrumented object were zeroed and their .gcda files deleted
    event CoverageReset(
      objects: U32 @< Instrumented objects in the gcov information section
    ) severity activity low id 5 format "coverage counters zeroed: {} objects"

    @ A window was rejected (EXECUTION_ERROR) and why
    event CoverageRejected(
      reason: DumpReject @< Rejection reason
    ) severity warning low id 6 format "coverage dump rejected: {}"

  }

}
