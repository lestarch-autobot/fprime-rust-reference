// ======================================================================
// \title  RustMonitorTester.hpp
// \author ubuntu
// \brief  hpp file for RustMonitor component test harness implementation class
// ======================================================================

#ifndef FprimeRustReference_RustMonitorTester_HPP
#define FprimeRustReference_RustMonitorTester_HPP

#include "FprimeRustReference/Components/RustMonitor/RustMonitor.hpp"
#include "FprimeRustReference/Components/RustMonitor/RustMonitorGTestBase.hpp"

namespace FprimeRustReference {

class RustMonitorTester final : public RustMonitorGTestBase {

public:
  // ----------------------------------------------------------------------
  // Constants
  // ----------------------------------------------------------------------

  // Maximum size of histories storing events, telemetry, and port outputs
  static const FwSizeType MAX_HISTORY_SIZE = 32;  // UT-RM-5 records 11 responses and 10 events

  // Instance ID supplied to the component instance under test
  static const FwEnumStoreType TEST_INSTANCE_ID = 0;

  // Queue depth supplied to the component instance under test
  static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

public:
  // ----------------------------------------------------------------------
  // Construction and destruction
  // ----------------------------------------------------------------------

  //! Construct object RustMonitorTester
  RustMonitorTester();

  //! Destroy object RustMonitorTester
  ~RustMonitorTester();

public:
  // ----------------------------------------------------------------------
  // Tests
  // ----------------------------------------------------------------------

  //! To do
  //! UT-RM-1: SET_LIMIT(4) then run
  void setLimit();
  //! UT-RM-2: SET_LIMIT(0) rejected, limit unchanged
  void setLimitZeroRejected();
  //! UT-RM-3: counts below and above the limit
  void countsAgainstLimit();
  //! UT-RM-4: run drains the queue before run_handler, RunCount and Limit published
  void runDrainsQueue();

  //! UT-RM-5: eleventh SET_LIMIT on a full queue is answered BUSY by the overflow hook
  void setLimitBusyOnFullQueue();
private:
  void run();

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
  RustMonitor component;
};

} // namespace FprimeRustReference

#endif
