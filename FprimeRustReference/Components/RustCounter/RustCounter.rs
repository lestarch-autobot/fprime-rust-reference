//! `FprimeRustReference.RustCounter` implementation: a saturating counter driven by commands and the rate group

use crate::rustcounter_base::*;

/// State of `FprimeRustReference.RustCounter`
#[derive(Debug, Default)]
pub struct RustCounter {
    counter: u32,
    tick_count: u32,
}

impl RustCounter {
    /// Publish the counter and send it on `countOut` (REQ-RustCounter-004)
    fn report(&self, ctx: &mut Context<'_>) {
        ctx.tlmWrite_Counter(self.counter);
        ctx.countOut_out(0, self.counter);
    }
}

impl Component for RustCounter {
    fn new() -> Self {
        RustCounter::default()
    }

    fn tick_handler(&mut self, ctx: &mut Context<'_>, port_num: FwIndexType, context: u32) {
        let _ = (port_num, context);
        self.tick_count = self.tick_count.wrapping_add(1);
        ctx.tlmWrite_TickCount(self.tick_count);
        ctx.tlmWrite_Counter(self.counter);
    }

    fn RESET_cmd_handler(&mut self, ctx: &mut Context<'_>) -> CmdResponse {
        self.counter = 0;
        ctx.log_ACTIVITY_LO_ResetIssued();
        self.report(ctx);
        CmdResponse::Ok
    }

    fn BUMP_cmd_handler(&mut self, ctx: &mut Context<'_>, amount: u32) -> CmdResponse {
        let (threshold, _valid) = ctx.paramGet_THRESHOLD();
        let sum = self.counter.saturating_add(amount);
        if sum > threshold {
            self.counter = threshold;
            ctx.log_WARNING_LO_Saturated(self.counter, threshold);
        } else {
            self.counter = sum;
            ctx.log_ACTIVITY_LO_Bumped(self.counter);
        }
        self.report(ctx);
        CmdResponse::Ok
    }
}
