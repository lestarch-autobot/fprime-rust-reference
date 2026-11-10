####
# F prime CMakeLists.txt for the ReferenceDeployment topology.
####
add_compile_options(
  -Wno-shadow
)

register_fprime_module(
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/instances.fpp"
    "${CMAKE_CURRENT_LIST_DIR}/topology.fpp"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/FprimeRustReferenceTopology.cpp"
  DEPENDS
    Fw_Logger
    Svc_PosixTime
    Svc_TlmPacketizer_config_TlmPacketizerConfig
    FprimeRustReference_Components_RustCounter
    FprimeRustReference_Components_RustMonitor
    FprimeRustReference_Components_Relay
)
