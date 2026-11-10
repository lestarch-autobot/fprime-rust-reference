# Reference override of Svc/Subtopologies/CdhCore/CdhCoreConfig/CdhCoreConfig.fpp (same file name; see user manual, subtopologies)
module CdhCoreConfig {
    #Base ID for the CdhCore Subtopology, all components are offsets from this base ID
    constant BASE_ID = 0x01000000
    
    module QueueSizes {
        # 20: a Rust queued component draining its 10-deep queue answers up to 10 commands in one burst, and the
        # dispatcher must hold those responses plus the dispatches in flight (reference override, DEVELOPMENT iteration 2)
        constant cmdDisp     = 20
        constant events      = 10
        constant tlmSend     = 10
        constant $health     = 25
    }
    

    module StackSizes {
        constant cmdDisp     = 64 * 1024
        constant events      = 64 * 1024
        constant tlmSend     = 64 * 1024
    }

    module Priorities {
        constant cmdDisp     = 35
        constant $health     = 24
        constant events      = 23
        constant tlmSend     = 22

    }

    module CpuAffinities {
        constant cmdDisp     = Os.TASK_DEFAULT
        constant events      = Os.TASK_DEFAULT
        constant tlmSend     = Os.TASK_DEFAULT
    }
}
