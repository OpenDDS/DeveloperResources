cmake_minimum_required(VERSION 3.27)

project(opendds_tms CXX)
enable_testing()

find_package(OpenDDS REQUIRED)

add_library(OpenDDS_TMS
  common/Handshaking.cpp
  common/PowerDevice.cpp
  common/DeviceInfoDataReaderListenerImpl.cpp
  common/HeartbeatDataReaderListenerImpl.cpp
  common/QosHelper.cpp
)
target_include_directories(OpenDDS_TMS PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
opendds_export_header(OpenDDS_TMS DIR common)
opendds_target_sources(OpenDDS_TMS
  PUBLIC
    common/mil-std-3071_data_model.idl
  INCLUDE_BASE ${CMAKE_CURRENT_SOURCE_DIR}
  OPENDDS_IDL_OPTIONS -Lc++11
)
target_link_libraries(OpenDDS_TMS PUBLIC OpenDDS::Rtps_Udp)
target_compile_features(OpenDDS_TMS PUBLIC cxx_std_17)
opendds_bigobj(OpenDDS_TMS)

add_library(Commands_Idl)
opendds_target_sources(Commands_Idl
  PUBLIC
    cli_idl/CLICommands.idl
  OPENDDS_IDL_OPTIONS -Lc++11 -I${CMAKE_CURRENT_SOURCE_DIR}
  INCLUDE_BASE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(Commands_Idl PUBLIC OpenDDS_TMS)

add_executable(Controller
  controller/main.cpp
  controller/Controller.cpp
  controller/CLIServer.cpp
  controller/PowerDevicesRequestDataReaderListenerImpl.cpp
  controller/OperatorIntentRequestDataReaderListenerImpl.cpp
  controller/ControllerCommandDataReaderListenerImpl.cpp
)
target_include_directories(Controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Controller PRIVATE Commands_Idl)

add_executable(CLI
  cli/main.cpp
  cli/CLIClient.cpp
)
target_include_directories(CLI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(CLI PRIVATE Commands_Idl)

add_subdirectory(tests)
