####
# fprime-rust-reference root CMakeLists.txt
#
# Sets up the build for the FprimeRustReference project, which demonstrates
# the fprime-rust autocoder by hosting a Rust-backed component
# (``RustExample``) inside a standard F Prime topology.  This file is
# intentionally minimal -- the F Prime CMake API does the heavy lifting
# once the autocoder target has been registered.
####
cmake_minimum_required(VERSION 3.24.2)
project(FprimeRustReference C CXX)


###
# F' Core Setup
# Pulls in F Prime's CMake API (FPrime.cmake) which is responsible for
# loading ``settings.ini`` and arranging the build graph.
###
set(FPRIME_ENABLE_JSON_MODEL_GENERATION ON)
include("${CMAKE_CURRENT_LIST_DIR}/lib/fprime/cmake/FPrime.cmake")

# Register the fprime-rust autocoder target.  This must happen *between*
# ``FPrime.cmake`` (which defines the API macros) and ``fprime_setup_included_code()``
# (which kicks off module detection and locks the target list).  The
# pattern matches what fprime-python-reference does for fprime-python.
if (NOT DEFINED FPRIME_SUB_BUILD_TARGETS)
    register_fprime_target(target/fprime_rust)
endif()


# NOTE: register custom targets between these two lines
fprime_setup_included_code()


# This includes project-wide objects
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/FprimeRustReference")
