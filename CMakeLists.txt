####
# fprime-rust-reference root CMakeLists.txt
#
# Sets up the build for the FprimeRustReference project, which demonstrates
# the fprime-rust autocoder by hosting two Rust-backed components
# (``RustCounter``, ``RustMonitor``) and a C++ ``Relay`` inside a standard F Prime topology.  This file is
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
include("${CMAKE_CURRENT_LIST_DIR}/lib/fprime/cmake/FPrime.cmake")

# Set up the Rust integration: registers the fprime_rust build target.  This must happen *between*
# ``FPrime.cmake`` and ``fprime_setup_included_code()``, the same shape as fprime-python-reference's
# ``include(FprimePython)``.  No FPRIME_ENABLE_JSON_MODEL_GENERATION: fprime-rust reads the FPP model
# in-process (fprime-fpp-python); no JSON model is generated or read.
include(FprimeRust)


# NOTE: register custom targets between these two lines
fprime_setup_included_code()


# This includes project-wide objects
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/FprimeRustReference")
