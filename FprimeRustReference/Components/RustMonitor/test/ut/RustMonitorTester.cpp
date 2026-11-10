// ======================================================================
// \title  RustMonitorTester.cpp
// \author ubuntu
// \brief  cpp file for RustMonitor component test harness implementation class
// ======================================================================

#include <limits>
#include "RustMonitorTester.hpp"
#include "Fw/Test/UnitTest.hpp"

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
    REQUIREMENT("REQ-RustMonitor-001");
    COMMENT("UT-RM-1: SET_LIMIT(4) then run: LimitSet(4), Limit 4, response OK");
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
    REQUIREMENT("REQ-RustMonitor-002");
    REQUIREMENT("REQ-RustMonitor-006");
    COMMENT("UT-RM-2: SET_LIMIT(0) then run: InvalidLimit(0), response VALIDATION_ERROR, Limit unchanged (4294967295)");
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
    REQUIREMENT("REQ-RustMonitor-003");
    REQUIREMENT("REQ-RustMonitor-004");
    COMMENT("UT-RM-3: countIn(3) then run with limit 4: LastCount 3, no LimitExceeded; countIn(10) then run: LimitExceeded(10, 4), Exceedances 1");
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
    REQUIREMENT("REQ-RustMonitor-005");
    COMMENT("UT-RM-4: two counts queued, one run: both handled before run_handler, RunCount 1, Limit published; run with an empty queue: RunCount 2");
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
