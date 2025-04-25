cmake_minimum_required(VERSION 3.27)

project(opendds_tms CXX)
enable_testing()

find_package(OpenDDS REQUIRED)

add_library(TMS_Common
  common/Handshaking.cpp
  common/PowerDevice.cpp
  common/DeviceInfoDataReaderListenerImpl.cpp
  common/HeartbeatDataReaderListenerImpl.cpp
  common/QosHelper.cpp
)
target_include_directories(TMS_Common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
opendds_export_header(TMS_Common INCLUDE "common/OpenDDS_TMS_export.h" MACRO_PREFIX OpenDDS_TMS)
opendds_target_sources(TMS_Common
  PUBLIC
    common/mil-std-3071_data_model.idl
  INCLUDE_BASE ${CMAKE_CURRENT_SOURCE_DIR}
  OPENDDS_IDL_OPTIONS -Lc++11
)
target_link_libraries(TMS_Common PUBLIC OpenDDS::Rtps_Udp)
target_compile_features(TMS_Common PUBLIC cxx_std_17)
opendds_bigobj(TMS_Common)

add_library(Commands_Idl)
opendds_target_sources(Commands_Idl
  PUBLIC
    cli_idl/CLICommands.idl
  OPENDDS_IDL_OPTIONS -Lc++11 -I${CMAKE_CURRENT_SOURCE_DIR}
  INCLUDE_BASE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(Commands_Idl PUBLIC TMS_Common)

add_executable(Controller
  controller/main.cpp
  controller/Controller.cpp
  controller/CLIServer.cpp
  controller/PowerDevicesRequestDataReaderListenerImpl.cpp
  controller/OperatorIntentRequestDataReaderListenerImpl.cpp
  controller/ControllerCommandDataReaderListenerImpl.cpp
  controller/ReplyDataReaderListenerImpl.cpp
)
target_include_directories(Controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Controller PRIVATE Commands_Idl)

add_executable(CLI
  cli/main.cpp
  cli/CLIClient.cpp
)
target_include_directories(CLI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(CLI PRIVATE Commands_Idl)

add_executable(Source
  power_devices/Source.cpp
)
target_include_directories(Source PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Source PRIVATE TMS_Common)

add_executable(Load
  power_devices/Load.cpp
)
target_include_directories(Load PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Load PRIVATE TMS_Common)

add_subdirectory(tests)
