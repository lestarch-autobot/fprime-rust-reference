module FprimeRustReference {

  # ----------------------------------------------------------------------
  # Base ID Convention
  # ----------------------------------------------------------------------
  #
  # All Base IDs follow the 8-digit hex format: 0xDSSCCxxx
  #
  # Where:
  #   D   = Deployment digit (1-F)
  #   SS  = Subtopology digits (00 for main topology, 01-FF)
  #   CC  = Component digits (00-FF)
  #   xxx = Reserved for internal component items (events, commands, telemetry)
  #
  # The reference deployment intentionally keeps a tiny set of instances --
  # just what is required to drive ``rustCounter``, ``rustMonitor`` and ``relay`` from fprime-gds and watch
  # its commands, events, and telemetry round-trip.  Real deployments will
  # add their own instances alongside the ones below.

  # ----------------------------------------------------------------------
  # Defaults
  # ----------------------------------------------------------------------

  module Default {
    constant QUEUE_SIZE = 10
    constant STACK_SIZE = 64 * 1024
  }

  # ----------------------------------------------------------------------
  # Active component instances
  # ----------------------------------------------------------------------

  instance rateGroup1Comp: Svc.ActiveRateGroup base id 0x10001000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 43

  instance rateGroup2Comp: Svc.ActiveRateGroup base id 0x10002000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 42

  instance rateGroup3Comp: Svc.ActiveRateGroup base id 0x10003000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 41

  # The Rust-backed reference component instances.  RustCounter handlers live in
  # ``FprimeRustReference/Components/RustCounter/RustCounter.rs`` and the C++
  # shim is generated at build time by the ``fprime-rust`` autocoder.

  instance rustCounter: FprimeRustReference.RustCounter base id 0x10016000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 18

  # ----------------------------------------------------------------------
  # Queued component instances
  # ----------------------------------------------------------------------

  instance rustMonitor: FprimeRustReference.RustMonitor base id 0x10017000 \
    queue size Default.QUEUE_SIZE

  # ----------------------------------------------------------------------
  # Passive component instances
  # ----------------------------------------------------------------------

  instance posixTime: Svc.PosixTime base id 0x10020000

  instance rateGroupDriverComp: Svc.RateGroupDriver base id 0x10021000

  instance systemResources: Svc.SystemResources base id 0x10023000

  instance linuxTimer: Svc.LinuxTimer base id 0x10024000

  instance comDriver: Drv.TcpClient base id 0x10025000
  instance relay: FprimeRustReference.Relay base id 0x10026000

  @ Harness tester: run-start markers and coverage dumps (not UUT)
  instance rustTester: FprimeRustReference.RustTester base id 0x10027000

}
