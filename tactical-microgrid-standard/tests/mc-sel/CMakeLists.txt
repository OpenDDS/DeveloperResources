cmake_minimum_required(VERSION 3.27)

project(opendds_tms_mc_sel_test CXX)
enable_testing()

find_package(OpenDDS REQUIRED)
include(opendds_testing)

add_executable(mc-sel-test mc-sel-test.cpp)
target_link_libraries(mc-sel-test PRIVATE OpenDDS_TMS)

add_executable(basic-mc ${CMAKE_SOURCE_DIR}/controller/Controller.cpp basic-mc.cpp)
target_link_libraries(basic-mc PRIVATE OpenDDS_TMS)

opendds_add_test(COMMAND ./mc-sel-test)
