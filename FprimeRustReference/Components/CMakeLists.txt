# Project-specific components.  RustExample is the fprime-rust demo;
# the others are inherited unchanged from the upstream Ref deployment so
# that the topology has a non-trivial set of producers / consumers around
# the Rust component.
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/RustExample")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/BlockDriver")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/DpDemo")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/PingReceiver")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/RecvBuffApp")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/SendBuffApp")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/SignalGen")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/TypeDemo")
