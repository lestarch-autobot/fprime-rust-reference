# Project-specific components.
#
# This reference deployment intentionally hosts just one component --
# ``RustExample`` -- so the topology stays a minimal, narrowly scoped
# demonstration of the ``fprime-rust`` autocoder.  Real deployments will
# add their own ``add_fprime_subdirectory`` calls below.
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/RustExample")
