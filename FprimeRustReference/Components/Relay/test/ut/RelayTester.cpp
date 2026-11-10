// ======================================================================
// \title  RelayTester.cpp
// \author ubuntu
// \brief  cpp file for Relay component test harness implementation class
// ======================================================================

#include "RelayTester.hpp"

namespace FprimeRustReference {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

RelayTester::RelayTester()
    : RelayGTestBase("RelayTester", RelayTester::MAX_HISTORY_SIZE),
      component("Relay") {
  this->initComponents();
  this->connectPorts();
}

RelayTester::~RelayTester() { this->component.deinit(); }

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void RelayTester::forwardsCount() {
    this->invoke_to_countIn(0, 7);
    ASSERT_from_countOut_SIZE(1);
    ASSERT_from_countOut(0, 7);
}

void RelayTester::relayedTelemetry() {
    for (U32 i = 1; i <= 3; i++) {
        this->invoke_to_countIn(0, i);
        ASSERT_TLM_Relayed_SIZE(i);
        ASSERT_TLM_Relayed(i - 1, i);
    }
    ASSERT_from_countOut_SIZE(3);
}

void RelayTester::unconnectedOutput() {
    // A second instance whose countOut is left unconnected; telemetry and time go to this tester
    Relay bare("bare");
    bare.init(0);
    bare.set_tlmOut_OutputPort(0, this->get_from_tlmOut(0));
    bare.set_timeCaller_OutputPort(0, this->get_from_timeCaller(0));
    bare.get_countIn_InputPort(0)->invoke(9);
    ASSERT_from_countOut_SIZE(0);
    ASSERT_TLM_Relayed_SIZE(1);
    ASSERT_TLM_Relayed(0, 1);
}

} // namespace FprimeRustReference
