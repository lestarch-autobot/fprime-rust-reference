// ======================================================================
// \title  RustCounterTester.hpp
// \author ubuntu
// \brief  hpp file for RustCounter component test harness implementation class
// ======================================================================

#ifndef FprimeRustReference_RustCounterTester_HPP
#define FprimeRustReference_RustCounterTester_HPP

#include "FprimeRustReference/Components/RustCounter/RustCounter.hpp"
#include "FprimeRustReference/Components/RustCounter/RustCounterGTestBase.hpp"

namespace FprimeRustReference {

class RustCounterTester final : public RustCounterGTestBase {

public:
  // ----------------------------------------------------------------------
  // Constants
  // ----------------------------------------------------------------------

  // Maximum size of histories storing events, telemetry, and port outputs
  static const FwSizeType MAX_HISTORY_SIZE = 10;

  // Instance ID supplied to the component instance under test
  static const FwEnumStoreType TEST_INSTANCE_ID = 0;

  // Queue depth supplied to the component instance under test
  static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

public:
  // ----------------------------------------------------------------------
  // Construction and destruction
  // ----------------------------------------------------------------------

  //! Construct object RustCounterTester
  RustCounterTester();

  //! Destroy object RustCounterTester
  ~RustCounterTester();

public:
  // ----------------------------------------------------------------------
  // Tests
  // ----------------------------------------------------------------------

  //! To do
  //! UT-RC-1: RESET after BUMP(5)
  void resetAfterBump();
  //! UT-RC-2: THRESHOLD 10 bump sequence
  void bumpWithinAndPastThreshold();
  //! UT-RC-3: BUMP(U32 max) saturates at the default threshold without wrapping
  void bumpMaxNoWrap();
  //! UT-RC-4: THRESHOLD lowered below the counter takes effect on the next BUMP
  void thresholdUpdateTakesEffect();
  //! UT-RC-6: ThresholdValid DEFAULT before any set, VALID after a set
  void thresholdValidityReported();
  //! UT-RC-5: three ticks
  void ticks();
private:
  void bump(U32 amount);
  void reset();

private:
  // ----------------------------------------------------------------------
  // Helper functions
  // ----------------------------------------------------------------------

  //! Connect ports
  void connectPorts();

  //! Initialize components
  void initComponents();

private:
  // ----------------------------------------------------------------------
  // Member variables
  // ----------------------------------------------------------------------

  //! The component under test
  RustCounter component;
};

} // namespace FprimeRustReference

#endif
