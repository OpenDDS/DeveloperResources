cmake_minimum_required(VERSION 3.27)

project(opendds_tms CXX)
enable_testing()

find_package(OpenDDS REQUIRED)

add_library(TMS_Common
  common/Handshaking.cpp
  common/ControllerSelector.cpp
  common/DeviceInfoDataReaderListenerImpl.cpp
  common/HeartbeatDataReaderListenerImpl.cpp
  common/QosHelper.cpp
  common/Utils.cpp
)
target_include_directories(TMS_Common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
opendds_export_header(TMS_Common INCLUDE "common/OpenDDS_TMS_export.h" MACRO_PREFIX OpenDDS_TMS)
opendds_target_sources(TMS_Common
  PUBLIC
    common/mil-std-3071_data_model.idl
  INCLUDE_BASE ${CMAKE_CURRENT_SOURCE_DIR}
  OPENDDS_IDL_OPTIONS -Lc++11 -Gxtypes-complete
)
target_link_libraries(TMS_Common PUBLIC OpenDDS::Rtps_Udp)
target_compile_features(TMS_Common PUBLIC cxx_std_17)
opendds_bigobj(TMS_Common)

add_library(Commands_Idl)
opendds_target_sources(Commands_Idl
  PUBLIC
    cli_idl/CLICommands.idl
  OPENDDS_IDL_OPTIONS -Lc++11 -Gxtypes-complete -I${CMAKE_CURRENT_SOURCE_DIR}
  INCLUDE_BASE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(Commands_Idl PUBLIC TMS_Common)

add_library(PowerSim_Idl
  power_devices/PowerDevice.cpp
  power_devices/PowerConnectionDataReaderListenerImpl.cpp
  power_devices/EnergyStartStopRequestDataReaderListenerImpl.cpp
)
opendds_export_header(PowerSim_Idl)
opendds_target_sources(PowerSim_Idl
  PUBLIC
    power_devices/PowerSim.idl
  OPENDDS_IDL_OPTIONS -Lc++11 -Gxtypes-complete -I${CMAKE_CURRENT_SOURCE_DIR}
  INCLUDE_BASE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(PowerSim_Idl PUBLIC TMS_Common)

add_executable(Controller
  controller/main.cpp
  controller/Controller.cpp
  controller/CLIServer.cpp
  controller/PowerDevicesRequestDataReaderListenerImpl.cpp
  controller/OperatorIntentRequestDataReaderListenerImpl.cpp
  controller/ControllerCommandDataReaderListenerImpl.cpp
  controller/ReplyDataReaderListenerImpl.cpp
  controller/PowerTopologyDataReaderListenerImpl.cpp
  controller/ActiveMicrogridControllerStateDataReaderListenerImpl.cpp
)
target_include_directories(Controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Controller PRIVATE Commands_Idl PowerSim_Idl)

add_executable(CLI
  cli/main.cpp
  cli/CLIClient.cpp
)
target_include_directories(CLI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(CLI PRIVATE Commands_Idl PowerSim_Idl)

add_executable(Source
  power_devices/Source.cpp
)
target_include_directories(Source PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Source PRIVATE PowerSim_Idl)

add_executable(Load
  power_devices/Load.cpp
)
target_include_directories(Load PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Load PRIVATE PowerSim_Idl)

add_executable(Distribution
  power_devices/Distribution.cpp
)
target_include_directories(Distribution PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Distribution PRIVATE PowerSim_Idl)

# Generated code with complete type objects enabled for the TMS IDL file
# causes stack overflow on Windows.
# Increase the stack size to 2MB (default is 1MB).
if (MSVC)
  target_link_options(Controller PRIVATE "/STACK:2097152")
  target_link_options(CLI PRIVATE "/STACK:2097152")
  target_link_options(Source PRIVATE "/STACK:2097152")
  target_link_options(Load PRIVATE "/STACK:2097152")
  target_link_options(Distribution PRIVATE "/STACK:2097152")
endif()


add_subdirectory(tests)
