cmake_minimum_required(VERSION 3.27)

project(opendds_tms_mc_sel CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

add_executable(basic-dev basic-dev.cpp)
target_link_libraries(basic-dev PRIVATE PowerSim_Idl)

add_executable(basic-mc ${CMAKE_SOURCE_DIR}/controller/Controller.cpp basic-mc.cpp)
target_link_libraries(basic-mc PRIVATE Commands_Idl)

opendds_add_test(ARGS remove_dcs_after=0 test_verbose=1)
