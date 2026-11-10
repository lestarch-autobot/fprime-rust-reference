####
# FprimeRustReference/Components/RustTester: harness tester (markers, coverage dumps)
#
# `fprime_rust_reference_instrument(<target>)` (Components/CMakeLists.txt) adds the gcov
# instrumentation when FPRIME_RUST_REFERENCE_COVERAGE is ON; the tester's own dump path is
# excluded per function with `no_profile_instrument_function`.
####
register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/RustTester.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/RustTester.cpp"
    HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/RustTester.hpp"
)
fprime_rust_reference_instrument("${FPRIME_CURRENT_MODULE}")

register_fprime_ut(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/RustTester.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/RustTesterTester.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/RustTesterTestMain.cpp"
    DEPENDS
        FprimeRustReference_Components_RustTester
        STest
    UT_AUTO_HELPERS
)
