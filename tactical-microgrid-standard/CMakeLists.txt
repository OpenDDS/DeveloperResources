cmake_minimum_required(VERSION 3.27)

project(opendds_tms CXX)
enable_testing()

find_package(OpenDDS REQUIRED)

add_library(Common
  common/Handshaking.cpp
  common/PowerDevice.cpp
  common/DeviceInfoDataReaderListenerImpl.cpp
  common/HeartbeatDataReaderListenerImpl.cpp
  common/QosHelper.cpp
)
target_include_directories(Common PUBLIC ${CMAKE_SOURCE_DIR})
opendds_target_sources(Common
  PUBLIC
    idl/mil-std-3071_data_model.idl
  INCLUDE_BASE ${CMAKE_SOURCE_DIR}
  OPENDDS_IDL_OPTIONS -Lc++11
  USE_EXPORT "common/opendds_tms_export.h" OpenDDS_TMS_Export
)
target_link_libraries(Common PUBLIC OpenDDS::Rtps_Udp)
target_compile_features(Common PUBLIC cxx_std_17)

add_executable(Controller
  controller/ControllerMain.cpp
  controller/Controller.cpp
  controller/CLIServer.cpp
  controller/PowerDevicesRequestDataReaderListenerImpl.cpp
  controller/OperatorIntentRequestDataReaderListenerImpl.cpp
  controller/ControllerCommandDataReaderListenerImpl.cpp
)
target_include_directories(Controller PUBLIC ${CMAKE_SOURCE_DIR})
opendds_target_sources(Controller
  PRIVATE
    controller/CLICommands.idl
  INCLUDE_BASE ${CMAKE_SOURCE_DIR}
  OPENDDS_IDL_OPTIONS -Lc++11 -I${CMAKE_SOURCE_DIR}
)
target_link_libraries(Controller PRIVATE Common)

add_executable(CLI
  controller/CLIMain.cpp
  controller/CLIClient.cpp
)
target_include_directories(CLI PUBLIC ${CMAKE_SOURCE_DIR})
opendds_target_sources(CLI
  PRIVATE
    controller/CLICommands.idl
  INCLUDE_BASE ${CMAKE_SOURCE_DIR}
  OPENDDS_IDL_OPTIONS -Lc++11 -I${CMAKE_SOURCE_DIR}
)
target_link_libraries(CLI PRIVATE Common)

add_subdirectory(tests)
