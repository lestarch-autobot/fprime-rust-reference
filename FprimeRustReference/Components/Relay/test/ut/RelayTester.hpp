// ======================================================================
// \title  RelayTester.hpp
// \author ubuntu
// \brief  hpp file for Relay component test harness implementation class
// ======================================================================

#ifndef FprimeRustReference_RelayTester_HPP
#define FprimeRustReference_RelayTester_HPP

#include "FprimeRustReference/Components/Relay/Relay.hpp"
#include "FprimeRustReference/Components/Relay/RelayGTestBase.hpp"

namespace FprimeRustReference {

class RelayTester final : public RelayGTestBase {

public:
  // ----------------------------------------------------------------------
  // Constants
  // ----------------------------------------------------------------------

  // Maximum size of histories storing events, telemetry, and port outputs
  static const FwSizeType MAX_HISTORY_SIZE = 10;

  // Instance ID supplied to the component instance under test
  static const FwEnumStoreType TEST_INSTANCE_ID = 0;

public:
  // ----------------------------------------------------------------------
  // Construction and destruction
  // ----------------------------------------------------------------------

  //! Construct object RelayTester
  RelayTester();

  //! Destroy object RelayTester
  ~RelayTester();

public:
  // ----------------------------------------------------------------------
  // Tests
  // ----------------------------------------------------------------------

  //! To do
  //! UT-Relay-1: one countIn(7) produces one countOut(7)
  void forwardsCount();
  //! UT-Relay-2: Relayed = 1, 2, 3
  void relayedTelemetry();
  //! UT-Relay-2: countOut unconnected, count dropped, Relayed still increments
  void unconnectedOutput();

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
  Relay component;
};

} // namespace FprimeRustReference

#endif
