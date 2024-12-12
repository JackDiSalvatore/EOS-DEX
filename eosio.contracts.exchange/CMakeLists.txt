cmake_minimum_required(VERSION 3.5)

project(eosio_contracts)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 7)
set(VERSION_PATCH 0)
#set(VERSION_SUFFIX develop)

if (VERSION_SUFFIX)
   set(VERSION_FULL "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${VERSION_SUFFIX}")
else()
   set(VERSION_FULL "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
endif()

include(ExternalProject)

if (NOT EOSIO_CDT_ROOT)
   if (APPLE)
      set(EOSIO_CDT_ROOT /usr/local/Cellar/eosio.cdt/1.6.3)
   else()
      set(EOSIO_CDT_ROOT /usr/opt/eosio.cdt/1.6.3)
   endif()
endif()

if (NOT EXISTS ${EOSIO_CDT_ROOT}/lib/cmake/eosio.cdt/EosioWasmToolchain.cmake)
   message(FATAL_ERROR "eosio.cdt not found, Please specify EOSIO_CDT_ROOT to the eosio.cdt install path")
endif()

message(STATUS "Building eosio.contracts v${VERSION_FULL}")

set(EOSIO_CDT_VERSION_MIN "1.6")
set(EOSIO_CDT_VERSION_SOFT_MAX "1.6")
#set(EOSIO_CDT_VERSION_HARD_MAX "")

set(EOS_BUILD_DIR "/eos/build" CACHE STRING "The EOS build directory")

ExternalProject_Add(
   contracts_project
   SOURCE_DIR ${CMAKE_SOURCE_DIR}/contracts
   BINARY_DIR ${CMAKE_BINARY_DIR}/contracts
   CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${EOSIO_CDT_ROOT}/lib/cmake/eosio.cdt/EosioWasmToolchain.cmake
   -DCMAKE_PREFIX_PATH="${EOSIO_CDT_ROOT}"
   UPDATE_COMMAND ""
   PATCH_COMMAND ""
   TEST_COMMAND ""
   INSTALL_COMMAND ""
   BUILD_ALWAYS 1
)

enable_testing()
add_subdirectory(tests)
