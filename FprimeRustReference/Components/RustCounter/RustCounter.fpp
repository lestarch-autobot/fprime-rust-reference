module FprimeRustReference {

  @ Saturating counter implemented in Rust: commands, events, telemetry, a parameter,
  @ a rate-group input port (C++ to Rust) and a count output port (Rust to C++).
  @ fprime-rust
  active component RustCounter {

    # ----------------------------------------------------------------------
    # General ports
    # ----------------------------------------------------------------------

    @ Rate-group tick: increments TickCount and publishes telemetry
    async input port tick: Svc.Sched

    @ Reports the counter after every change
    output port countOut: FprimeRustReference.Count

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

    @ Parameter get port
    param get port prmGetOut

    @ Parameter set port
    param set port prmSetOut

    # ----------------------------------------------------------------------
    # Commands
    # ----------------------------------------------------------------------

    @ Reset the counter to zero
    async command RESET opcode 0

    @ Add amount to the counter, saturating at THRESHOLD
    async command BUMP(
      amount: U32 @< Amount to add
    ) opcode 1

    # ----------------------------------------------------------------------
    # Events
    # ----------------------------------------------------------------------

    @ Counter reset to zero
    event ResetIssued severity activity low id 0 format "RustCounter reset to 0"

    @ Counter changed by BUMP
    event Bumped(
      value: U32 @< New counter value
    ) severity activity low id 1 format "RustCounter bumped to {}"

    @ BUMP reached the THRESHOLD and saturated
    event Saturated(
      value: U32 @< Saturated counter value
      threshold: U32 @< THRESHOLD in effect
    ) severity warning low id 2 format "RustCounter saturated at {} (threshold {})"

    # ----------------------------------------------------------------------
    # Telemetry
    # ----------------------------------------------------------------------

    @ Current counter value
    telemetry Counter: U32 id 0

    @ Number of rate-group ticks received
    telemetry TickCount: U32 id 1

    @ Validity of THRESHOLD as read by the handler (REQ-RustCounter-007)
    telemetry ThresholdValid: Fw.ParamValid id 2

    # ----------------------------------------------------------------------
    # Parameters
    # ----------------------------------------------------------------------

    @ Maximum counter value; BUMP saturates here
    param THRESHOLD: U32 default 100 id 0

  }

}
