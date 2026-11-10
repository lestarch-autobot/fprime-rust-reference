#####
# 'ReferenceDeployment' Deployment:
#
# Registers the executable that wires the Rust-backed RustCounter and RustMonitor and the C++ Relay
# component into a complete F Prime topology.
#####

add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Top/")

register_fprime_deployment(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/Main.cpp"
    DEPENDS
        ${FPRIME_CURRENT_MODULE}_Top
)
