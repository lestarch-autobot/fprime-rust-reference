// ======================================================================
// \title  RustCounterTester.cpp
// \author ubuntu
// \brief  cpp file for RustCounter component test harness implementation class
// ======================================================================

#include <limits>
#include "RustCounterTester.hpp"
#include "Fw/Test/UnitTest.hpp"

namespace FprimeRustReference {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

RustCounterTester::RustCounterTester()
    : RustCounterGTestBase("RustCounterTester",
                           RustCounterTester::MAX_HISTORY_SIZE),
      component("RustCounter") {
  this->initComponents();
  this->connectPorts();
}

RustCounterTester::~RustCounterTester() { this->component.deinit(); }

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void RustCounterTester::bump(U32 amount) {
    this->sendCmd_BUMP(0, 10, amount);
    this->component.doDispatch();
}

void RustCounterTester::reset() {
    this->sendCmd_RESET(0, 11);
    this->component.doDispatch();
}

void RustCounterTester::resetAfterBump() {
    REQUIREMENT("REQ-RustCounter-001");
    REQUIREMENT("REQ-RustCounter-004");
    COMMENT("UT-RC-1: RESET after BUMP(5): ResetIssued, Counter 0, countOut(0), response OK");
    this->paramSet_THRESHOLD(100, Fw::ParamValid::VALID);
    this->component.loadParameters();
    this->bump(5);
    ASSERT_EVENTS_Bumped(0, 5);
    this->clearHistory();
    this->reset();
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_ResetIssued_SIZE(1);
    ASSERT_TLM_Counter_SIZE(1);
    ASSERT_TLM_Counter(0, 0);
    ASSERT_from_countOut_SIZE(1);
    ASSERT_from_countOut(0, 0);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, RustCounter::OPCODE_RESET, 11, Fw::CmdResponse::OK);
}

void RustCounterTester::bumpWithinAndPastThreshold() {
    REQUIREMENT("REQ-RustCounter-002");
    REQUIREMENT("REQ-RustCounter-003");
    COMMENT("UT-RC-2: THRESHOLD 10: BUMP(4) → Bumped(4); BUMP(6) → Bumped(10) (equality); BUMP(1) → Saturated(10, 10), counter 10; BUMP(0) → Bumped(10)");
    this->paramSet_THRESHOLD(10, Fw::ParamValid::VALID);
    this->component.loadParameters();
    this->bump(4);
    ASSERT_EVENTS_Bumped(0, 4);
    ASSERT_TLM_Counter(0, 4);
    ASSERT_from_countOut(0, 4);
    this->bump(6);
    ASSERT_EVENTS_Bumped(1, 10);
    ASSERT_EVENTS_Saturated_SIZE(0);
    this->bump(1);
    ASSERT_EVENTS_Saturated_SIZE(1);
    ASSERT_EVENTS_Saturated(0, 10, 10);
    ASSERT_TLM_Counter(2, 10);
    this->bump(0);
    ASSERT_EVENTS_Bumped_SIZE(3);
    ASSERT_EVENTS_Bumped(2, 10);
    ASSERT_from_countOut_SIZE(4);
    ASSERT_from_countOut(3, 10);
    ASSERT_CMD_RESPONSE_SIZE(4);
    ASSERT_CMD_RESPONSE(3, RustCounter::OPCODE_BUMP, 10, Fw::CmdResponse::OK);
}

void RustCounterTester::bumpMaxNoWrap() {
    REQUIREMENT("REQ-RustCounter-003");
    COMMENT("UT-RC-3: counter 1, BUMP(4294967295) → Saturated(100, 100), no panic");
    // No ground value: loadParameters applies the FPP default (100)
    this->component.loadParameters();
    this->bump(1);
    ASSERT_EVENTS_Bumped(0, 1);
    this->bump(std::numeric_limits<U32>::max());
    ASSERT_EVENTS_Saturated_SIZE(1);
    ASSERT_EVENTS_Saturated(0, 100, 100);
    ASSERT_TLM_Counter(1, 100);
}

void RustCounterTester::thresholdUpdateTakesEffect() {
    REQUIREMENT("REQ-RustCounter-006");
    COMMENT("UT-RC-4: THRESHOLD set to 3 while counter is 5: next BUMP(1) → Saturated(3, 3), counter 3");
    this->paramSet_THRESHOLD(100, Fw::ParamValid::VALID);
    this->component.loadParameters();
    this->bump(5);
    ASSERT_EVENTS_Bumped(0, 5);
    this->paramSet_THRESHOLD(3, Fw::ParamValid::VALID);
    // PRM_SET is handled synchronously in cmdIn_handlerBase: no dispatch needed
    this->paramSend_THRESHOLD(0, 12);
    this->bump(1);
    ASSERT_EVENTS_Saturated_SIZE(1);
    ASSERT_EVENTS_Saturated(0, 3, 3);
    ASSERT_TLM_Counter(1, 3);
}

void RustCounterTester::ticks() {
    REQUIREMENT("REQ-RustCounter-005");
    COMMENT("UT-RC-5: three tick calls: TickCount 1, 2, 3 and Counter written each time; no countOut");
    for (U32 i = 1; i <= 3; i++) {
        this->invoke_to_tick(0, 0);
        this->component.doDispatch();
        ASSERT_TLM_TickCount_SIZE(i);
        ASSERT_TLM_TickCount(i - 1, i);
        ASSERT_TLM_Counter_SIZE(i);
        ASSERT_TLM_Counter(i - 1, 0);
    }
    ASSERT_from_countOut_SIZE(0);
    ASSERT_EVENTS_SIZE(0);
}

} // namespace FprimeRustReference
