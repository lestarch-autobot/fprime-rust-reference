  telemetry packets Packets {

    # Rust reference channels: the two Rust components and the C++ relay between them.
    packet RustReference id 38 group 2 {
      rustCounter.Counter
      rustCounter.TickCount
      rustMonitor.RunCount
      rustMonitor.LastCount
      rustMonitor.Exceedances
      rustMonitor.Limit
      relay.Relayed
    }

  } omit {
  }
