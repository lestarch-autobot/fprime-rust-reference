register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/RustMonitor.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/RustMonitor.rs"
    DEPENDS
        FprimeRustReference_Ports
)
