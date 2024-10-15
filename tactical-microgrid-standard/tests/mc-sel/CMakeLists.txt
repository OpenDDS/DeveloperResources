cmake_minimum_required(VERSION 3.27)

project(opendds_tms_mc_sel_test CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

add_executable(mc-sel-test mc-sel-test.cpp)
target_link_libraries(mc-sel-test PUBLIC opendds_tms)

add_executable(basic-mc basic-mc.cpp)
target_link_libraries(basic-mc PUBLIC opendds_tms)

opendds_add_test(COMMAND ./mc-sel-test)
