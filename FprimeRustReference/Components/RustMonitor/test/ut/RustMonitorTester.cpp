// ======================================================================
// \title  RustMonitorTester.cpp
// \author ubuntu
// \brief  cpp file for RustMonitor component test harness implementation class
// ======================================================================

#include <limits>
#include "RustMonitorTester.hpp"

namespace FprimeRustReference {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

RustMonitorTester::RustMonitorTester()
    : RustMonitorGTestBase("RustMonitorTester",
                           RustMonitorTester::MAX_HISTORY_SIZE),
      component("RustMonitor") {
  this->initComponents();
  this->connectPorts();
}

RustMonitorTester::~RustMonitorTester() { this->component.deinit(); }

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void RustMonitorTester::run() {
    this->invoke_to_run(0, 0);
}

void RustMonitorTester::setLimit() {
    this->sendCmd_SET_LIMIT(0, 10, 4);
    ASSERT_EVENTS_SIZE(0);
    this->run();
    ASSERT_EVENTS_LimitSet_SIZE(1);
    ASSERT_EVENTS_LimitSet(0, 4);
    ASSERT_TLM_Limit(0, 4);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, RustMonitor::OPCODE_SET_LIMIT, 10, Fw::CmdResponse::OK);
}

void RustMonitorTester::setLimitZeroRejected() {
    this->sendCmd_SET_LIMIT(0, 10, 0);
    this->run();
    ASSERT_EVENTS_InvalidLimit_SIZE(1);
    ASSERT_EVENTS_InvalidLimit(0, 0);
    ASSERT_EVENTS_LimitSet_SIZE(0);
    ASSERT_CMD_RESPONSE(0, RustMonitor::OPCODE_SET_LIMIT, 10, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_TLM_Limit_SIZE(1);
    ASSERT_TLM_Limit(0, std::numeric_limits<U32>::max());
}

void RustMonitorTester::countsAgainstLimit() {
    this->sendCmd_SET_LIMIT(0, 10, 4);
    this->run();
    this->clearHistory();
    this->invoke_to_countIn(0, 3);
    this->run();
    ASSERT_TLM_LastCount_SIZE(1);
    ASSERT_TLM_LastCount(0, 3);
    ASSERT_EVENTS_LimitExceeded_SIZE(0);
    this->invoke_to_countIn(0, 10);
    this->run();
    ASSERT_TLM_LastCount(1, 10);
    ASSERT_EVENTS_LimitExceeded_SIZE(1);
    ASSERT_EVENTS_LimitExceeded(0, 10, 4);
    ASSERT_TLM_Exceedances_SIZE(1);
    ASSERT_TLM_Exceedances(0, 1);
}

void RustMonitorTester::runDrainsQueue() {
    this->invoke_to_countIn(0, 1);
    this->invoke_to_countIn(0, 2);
    ASSERT_TLM_LastCount_SIZE(0);
    this->run();
    ASSERT_TLM_LastCount_SIZE(2);
    ASSERT_TLM_LastCount(0, 1);
    ASSERT_TLM_LastCount(1, 2);
    ASSERT_TLM_RunCount_SIZE(1);
    ASSERT_TLM_RunCount(0, 1);
    ASSERT_TLM_Limit_SIZE(1);
    ASSERT_TLM_Limit(0, std::numeric_limits<U32>::max());
    this->run();
    ASSERT_TLM_RunCount(1, 2);
}

} // namespace FprimeRustReference
