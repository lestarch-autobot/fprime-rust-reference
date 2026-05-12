telemetry packets Packets {

  # The reference deployment ships a single project-specific telemetry
  # packet: ``RustExample`` carries the running counter that the
  # Rust-backed component updates from ``Bump`` / ``Reset``.
  packet RustExample id 38 group 2 {
    rustExample.Counter
  }

  # The CdhCore / ComCcsds / FileHandling / DataProducts subtopologies
  # publish a handful of housekeeping channels that we don't packetise
  # here -- they are routinely-collected diagnostic channels and the
  # GDS displays them on demand without packetisation.  The fpp-to-cpp
  # ``TlmPacketizer`` validator requires every channel in the topology
  # to be either packetised or explicitly omitted.

  packet RateGroups id 39 group 2 {
    rateGroup1Comp.RgMaxTime
    rateGroup2Comp.RgMaxTime
    rateGroup3Comp.RgMaxTime
    rateGroup1Comp.RgCycleSlips
    rateGroup2Comp.RgCycleSlips
    rateGroup3Comp.RgCycleSlips
  }

  packet SystemRes id 40 group 2 {
    systemResources.MEMORY_TOTAL
    systemResources.MEMORY_USED
    systemResources.NON_VOLATILE_TOTAL
    systemResources.NON_VOLATILE_FREE
    systemResources.CPU
    systemResources.CPU_00
    systemResources.CPU_01
    systemResources.CPU_02
    systemResources.CPU_03
    systemResources.CPU_04
    systemResources.CPU_05
    systemResources.CPU_06
    systemResources.CPU_07
    systemResources.CPU_08
    systemResources.CPU_09
    systemResources.CPU_10
    systemResources.CPU_11
    systemResources.CPU_12
    systemResources.CPU_13
    systemResources.CPU_14
    systemResources.CPU_15
  }

  packet CDH id 41 group 1 {
    CdhCore.cmdDisp.CommandsDispatched
    CdhCore.cmdDisp.CommandsDropped
    CdhCore.$health.PingLateWarnings

    FileHandling.fileUplink.FilesReceived
    FileHandling.fileUplink.PacketsReceived
    FileHandling.fileUplink.Warnings
    FileHandling.fileDownlink.FilesSent
    FileHandling.fileDownlink.PacketsSent
    FileHandling.fileDownlink.Warnings
    FileHandling.fileManager.CommandsExecuted
    FileHandling.fileManager.Errors

    ComCcsds.comQueue.comQueueDepth
    ComCcsds.comQueue.buffQueueDepth
    ComCcsds.commsBufferManager.TotalBuffs
    ComCcsds.commsBufferManager.CurrBuffs
    ComCcsds.commsBufferManager.HiBuffs
    ComCcsds.commsBufferManager.NoBuffs
    ComCcsds.commsBufferManager.EmptyBuffs
  }

  packet DataProducts id 42 group 3 {
    DataProducts.dpCat.CatalogDps
    DataProducts.dpCat.DpsSent

    DataProducts.dpMgr.NumSuccessfulAllocations
    DataProducts.dpMgr.NumFailedAllocations
    DataProducts.dpMgr.NumDataProducts
    DataProducts.dpMgr.NumBytes

    DataProducts.dpWriter.NumBuffersReceived
    DataProducts.dpWriter.NumBytesWritten
    DataProducts.dpWriter.NumSuccessfulWrites
    DataProducts.dpWriter.NumFailedWrites
    DataProducts.dpWriter.NumErrors

    DataProducts.dpBufferManager.TotalBuffs
    DataProducts.dpBufferManager.CurrBuffs
    DataProducts.dpBufferManager.HiBuffs
    DataProducts.dpBufferManager.NoBuffs
    DataProducts.dpBufferManager.EmptyBuffs
  }

  packet Version1 id 43 group 2 {
    CdhCore.version.FrameworkVersion
    CdhCore.version.ProjectVersion
  }

  packet Version_Library1 id 44 group 2 {
    CdhCore.version.LibraryVersion01
    CdhCore.version.LibraryVersion02
  }

  packet Version_Library2 id 45 group 2 {
    CdhCore.version.LibraryVersion03
    CdhCore.version.LibraryVersion04
  }

  packet Version_Library3 id 46 group 2 {
    CdhCore.version.LibraryVersion05
    CdhCore.version.LibraryVersion06
  }

  packet Version_Library4 id 47 group 2 {
    CdhCore.version.LibraryVersion07
    CdhCore.version.LibraryVersion08
  }

  packet Version_Library5 id 48 group 2 {
    CdhCore.version.LibraryVersion09
    CdhCore.version.LibraryVersion10
  }

  packet Version_Custom1 id 49 group 2 {
    CdhCore.version.CustomVersion01
  }

  packet Version_Custom2 id 50 group 2 {
    CdhCore.version.CustomVersion02
  }

  packet Version_Custom3 id 51 group 2 {
    CdhCore.version.CustomVersion03
  }

  packet Version_Custom4 id 52 group 2 {
    CdhCore.version.CustomVersion04
  }

  packet Version_Custom5 id 53 group 2 {
    CdhCore.version.CustomVersion05
  }

  packet Version_Custom6 id 54 group 2 {
    CdhCore.version.CustomVersion06
  }

  packet Version_Custom7 id 55 group 2 {
    CdhCore.version.CustomVersion07
  }

  packet Version_Custom8 id 56 group 2 {
    CdhCore.version.CustomVersion08
  }

  packet Version_Custom9 id 57 group 2 {
    CdhCore.version.CustomVersion09
  }

  packet Version_Custom10 id 58 group 2 {
    CdhCore.version.CustomVersion10
  }

} omit {
  CdhCore.cmdDisp.CommandErrors
}
