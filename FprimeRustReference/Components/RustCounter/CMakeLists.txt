register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/RustCounter.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/RustCounter.rs"
    DEPENDS
        FprimeRustReference_Ports
)
