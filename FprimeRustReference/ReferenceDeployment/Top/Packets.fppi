  telemetry packets Packets {

    packet CDH id 1 group 1 {
      CdhCore.cmdDisp.CommandsDispatched
      FileHandling.fileUplink.FilesReceived
      FileHandling.fileUplink.PacketsReceived
      FileHandling.fileDownlink.FilesSent
      FileHandling.fileDownlink.PacketsSent
      FileHandling.fileManager.CommandsExecuted
      ComCcsds.comQueue.comQueueDepth
      ComCcsds.comQueue.buffQueueDepth
      ComCcsds.commsBufferManager.TotalBuffs
      ComCcsds.commsBufferManager.CurrBuffs
      ComCcsds.commsBufferManager.HiBuffs
      rateGroup1Comp.RgMaxTime
      rateGroup2Comp.RgMaxTime
      rateGroup3Comp.RgMaxTime
    }

    packet CDHErrors id 2 group 1 {
      CdhCore.$health.PingLateWarnings
      FileHandling.fileUplink.Warnings
      FileHandling.fileDownlink.Warnings
      FileHandling.fileManager.Errors
      ComCcsds.commsBufferManager.NoBuffs
      ComCcsds.commsBufferManager.EmptyBuffs
      rateGroup1Comp.RgCycleSlips
      rateGroup2Comp.RgCycleSlips
      rateGroup3Comp.RgCycleSlips
      CdhCore.cmdDisp.CommandErrors
      CdhCore.cmdDisp.CommandsDropped
      CdhCore.events.EventsDropped
      ComCcsds.comQueue.comIngressDropped
      ComCcsds.comQueue.buffIngressDropped
      ComCcsds.comQueue.comManagedDropped
      ComCcsds.comQueue.buffManagedDropped
      FileHandling.fileUplink.FilesReceivedFailed
    }

    packet SystemRes1 id 5 group 2 {
      systemResources.MEMORY_TOTAL
      systemResources.MEMORY_USED
      systemResources.NON_VOLATILE_TOTAL
      systemResources.NON_VOLATILE_FREE
    }

    packet SystemRes3 id 6 group 2 {
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

    packet DataProducts id 21 group 3 {
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

    packet Version1 id 22 group 2 {
      CdhCore.version.FrameworkVersion
      CdhCore.version.ProjectVersion
    }

    packet Version_Library1 id 23 group 2 {
      CdhCore.version.LibraryVersion01
      CdhCore.version.LibraryVersion02
    }

    # Rust reference channels: the two Rust components and the C++ relay between them.
    packet RustReference id 38 group 2 {
      rustCounter.Counter
      rustCounter.ThresholdValid
      rustCounter.TickCount
      rustMonitor.RunCount
      rustMonitor.LastCount
      rustMonitor.Exceedances
      rustMonitor.Limit
      rustTester.BurstResponsesOk
      rustTester.BurstResponsesError
      relay.Relayed
    }

  } omit {
    CdhCore.version.CustomVersion01
    CdhCore.version.CustomVersion02
    CdhCore.version.CustomVersion03
    CdhCore.version.CustomVersion04
    CdhCore.version.CustomVersion05
    CdhCore.version.CustomVersion06
    CdhCore.version.CustomVersion07
    CdhCore.version.CustomVersion08
    CdhCore.version.CustomVersion09
    CdhCore.version.CustomVersion10
    CdhCore.version.LibraryVersion03
    CdhCore.version.LibraryVersion04
    CdhCore.version.LibraryVersion05
    CdhCore.version.LibraryVersion06
    CdhCore.version.LibraryVersion07
    CdhCore.version.LibraryVersion08
    CdhCore.version.LibraryVersion09
    CdhCore.version.LibraryVersion10
    DataProducts.dpBufferAccumulator.BA_NumQueuedBuffers
  }
