####
# FPRIME_RUST_REFERENCE_COVERAGE (default OFF): gcov-instrument the judged scope — the Rust components'
# generated C++ shims, Relay and RustTester (and the FprimeRust glue, see ReferenceDeployment/CMakeLists.txt)
# — with a freestanding-style info section (`-fprofile-info-section=gcov_info`, GCC >= 12) that RustTester
# walks with `__gcov_info_to_gcda` on DUMP_COVERAGE. OFF leaves every target untouched (the RfR image).
####
option(FPRIME_RUST_REFERENCE_COVERAGE "Instrument the Rust reference scope for RustTester DUMP_COVERAGE" OFF)

# Instrument one module of the judged scope. A target that does not exist yet (e.g. FprimeRust, created when the
# library is processed after the deployment's CMakeLists) is retried once at the end of the top-level directory;
# a target still missing then is a configuration error, not a silent gap in the coverage image.
function(fprime_rust_reference_instrument TARGET_NAME)
    if (NOT FPRIME_RUST_REFERENCE_COVERAGE)
        return()
    endif()
    # the list lives in the function: a deferred call runs in the top-level directory scope, not in this one
    set(FPRIME_RUST_REFERENCE_COVERAGE_FLAGS
        -fprofile-arcs -ftest-coverage -fprofile-info-section=gcov_info -fprofile-update=single)
    if (TARGET "${TARGET_NAME}")
        target_compile_options("${TARGET_NAME}" PRIVATE ${FPRIME_RUST_REFERENCE_COVERAGE_FLAGS})
        target_compile_definitions("${TARGET_NAME}" PRIVATE FPRIME_RUST_REFERENCE_COVERAGE=1)
        target_link_libraries("${TARGET_NAME}" PUBLIC gcov)
        message(STATUS "FPRIME_RUST_REFERENCE_COVERAGE: instrumented ${TARGET_NAME}")
    elseif (ARGC GREATER 1)
        message(FATAL_ERROR "FPRIME_RUST_REFERENCE_COVERAGE: target ${TARGET_NAME} does not exist")
    else()
        cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL fprime_rust_reference_instrument "${TARGET_NAME}" DEFERRED)
    endif()
endfunction()

# Project-specific components: two Rust components and the C++ counterpart they exchange counts with.
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/RustCounter")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/RustMonitor")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Relay")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/RustTester")
foreach(_comp RustCounter RustMonitor Relay)
    fprime_rust_reference_instrument("FprimeRustReference_Components_${_comp}")
endforeach()
