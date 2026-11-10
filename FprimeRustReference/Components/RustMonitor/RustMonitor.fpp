module FprimeRustReference {

  @ Queued component implemented in Rust: watches counts relayed by a C++ component
  @ against a commanded limit and runs from a rate group.
  @ fprime-rust
  queued component RustMonitor {

    # ----------------------------------------------------------------------
    # General ports
    # ----------------------------------------------------------------------

    @ Rate-group run: dispatches queued counts and publishes telemetry
    sync input port run: Svc.Sched

    @ Count relayed from the C++ Relay component
    async input port countIn: FprimeRustReference.Count drop

    # ----------------------------------------------------------------------
    # Special ports
    # ----------------------------------------------------------------------

    @ Time stamping for events and telemetry
    time get port timeCaller

    @ Standard command interfaces
    import Fw.Command

    @ Standard event interfaces
    import Fw.Event

    @ Standard telemetry interface
    import Fw.Channel

    # ----------------------------------------------------------------------
    # Commands
    # ----------------------------------------------------------------------

    @ Set the count limit; zero is rejected; on a full queue the overflow hook responds BUSY (no assert)
    async command SET_LIMIT(
      limit: U32 @< New limit, must be non-zero
    ) opcode 0 hook

    # ----------------------------------------------------------------------
    # Events
    # ----------------------------------------------------------------------

    @ Limit changed
    event LimitSet(
      limit: U32 @< Limit now in effect
    ) severity activity low id 0 format "RustMonitor limit set to {}"

    @ SET_LIMIT rejected
    event InvalidLimit(
      limit: U32 @< Rejected limit
    ) severity warning low id 1 format "RustMonitor rejected limit {}"

    @ A relayed count exceeded the limit
    event LimitExceeded(
      count: U32 @< Count received
      limit: U32 @< Limit in effect
    ) severity warning high id 2 format "RustMonitor count {} exceeds limit {}"

    # ----------------------------------------------------------------------
    # Telemetry
    # ----------------------------------------------------------------------

    @ Number of run calls received
    telemetry RunCount: U32 id 0

    @ Last count received
    telemetry LastCount: U32 id 1

    @ Number of counts that exceeded the limit
    telemetry Exceedances: U32 id 2

    @ Limit in effect
    telemetry Limit: U32 id 3

  }

}
