cmake_minimum_required( VERSION 3.5 )

find_package(eosio)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/contracts.hpp)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include(${CMAKE_CURRENT_LIST_DIR}/EosioTester.cmake)
add_subdirectory(eosio_contract_tests)
