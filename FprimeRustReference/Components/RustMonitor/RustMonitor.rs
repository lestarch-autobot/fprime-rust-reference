//! `FprimeRustReference.RustMonitor` implementation: a queued limit monitor fed by the C++ relay

use crate::rustmonitor_base::*;

/// State of `FprimeRustReference.RustMonitor`
#[derive(Debug)]
pub struct RustMonitor {
    limit: u32,
    run_count: u32,
    exceedances: u32,
}

impl Component for RustMonitor {
    fn new() -> Self {
        RustMonitor { limit: u32::MAX, run_count: 0, exceedances: 0 }
    }

    fn countIn_handler(&mut self, ctx: &mut Context<'_>, port_num: FwIndexType, count: u32) {
        let _ = port_num;
        ctx.tlmWrite_LastCount(count);
        if count > self.limit {
            self.exceedances = self.exceedances.wrapping_add(1);
            ctx.log_WARNING_HI_LimitExceeded(count, self.limit);
            ctx.tlmWrite_Exceedances(self.exceedances);
        }
    }

    fn run_handler(&mut self, ctx: &mut Context<'_>, port_num: FwIndexType, context: u32) {
        let _ = (port_num, context);
        self.run_count = self.run_count.wrapping_add(1);
        ctx.tlmWrite_RunCount(self.run_count);
        ctx.tlmWrite_Limit(self.limit);
    }

    fn SET_LIMIT_cmd_handler(&mut self, ctx: &mut Context<'_>, limit: u32) -> CmdResponse {
        if limit == 0 {
            ctx.log_WARNING_LO_InvalidLimit(limit);
            return CmdResponse::ValidationError;
        }
        self.limit = limit;
        ctx.log_ACTIVITY_LO_LimitSet(limit);
        ctx.tlmWrite_Limit(limit);
        CmdResponse::Ok
    }

    /// Queue full: answer BUSY on the caller's thread; nothing is dropped silently and nothing asserts
    fn SET_LIMIT_overflow_hook(_ctx: &mut HookContext<'_>, _opcode: FwOpcodeType, _cmd_seq: u32) -> CmdResponse {
        CmdResponse::Busy
    }
}
