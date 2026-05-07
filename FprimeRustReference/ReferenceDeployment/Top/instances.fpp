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

  instance blockDrv: FprimeRustReference.BlockDriver base id 0x10000000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 47

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

  instance pingRcvr: FprimeRustReference.PingReceiver base id 0x10004000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 23

  instance typeDemo: FprimeRustReference.TypeDemo base id 0x10005000

  instance cmdSeq: Svc.CmdSequencer base id 0x10006000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 20

  instance dpDemo: FprimeRustReference.DpDemo base id 0x0A10 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 19

  # ----------------------------------------------------------------------
  # Queued component instances
  # ----------------------------------------------------------------------

  instance sendBuffComp: FprimeRustReference.SendBuff base id 0x10010000 \
    queue size Default.QUEUE_SIZE

  instance SG1: FprimeRustReference.SignalGen base id 0x10011000 \
    queue size Default.QUEUE_SIZE

  instance SG2: FprimeRustReference.SignalGen base id 0x10012000 \
    queue size Default.QUEUE_SIZE

  instance SG3: FprimeRustReference.SignalGen base id 0x10013000 \
    queue size Default.QUEUE_SIZE

  instance SG4: FprimeRustReference.SignalGen base id 0x10014000 \
    queue size Default.QUEUE_SIZE

  instance SG5: FprimeRustReference.SignalGen base id 0x10015000 \
    queue size Default.QUEUE_SIZE

  # The Rust-backed reference component instance.  Its handlers live in
  # ``FprimeRustReference/Components/RustExample/RustExample.rs`` and the C++
  # shim is generated at build time by the ``fprime-rust`` autocoder.

  instance rustExample: FprimeRustReference.RustExample base id 0x10016000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 18

  # ----------------------------------------------------------------------
  # Passive component instances
  # ----------------------------------------------------------------------

  instance posixTime: Svc.PosixTime base id 0x10020000

  instance rateGroupDriverComp: Svc.RateGroupDriver base id 0x10021000

  instance recvBuffComp: FprimeRustReference.RecvBuff base id 0x10022000

  instance systemResources: Svc.SystemResources base id 0x10023000

  instance linuxTimer: Svc.LinuxTimer base id 0x10024000

  instance comDriver: Drv.TcpClient base id 0x10025000

}
