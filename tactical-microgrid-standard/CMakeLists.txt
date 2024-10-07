cmake_minimum_required(VERSION 3.27)

project(opendds_tms CXX)
enable_testing()

find_package(OpenDDS REQUIRED)

add_library(${PROJECT_NAME}
  common/Handshaking.cpp
  common/Controller.cpp
  common/PowerDevice.cpp
  common/DeviceInfoDataReaderListenerImpl.cpp
  common/HeartbeatDataReaderListenerImpl.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC common)
opendds_export_header(${PROJECT_NAME} USE_EXPORT_VAR export)
opendds_target_sources(${PROJECT_NAME}
  PUBLIC
    idl/mil-std-3071_data_model.idl
  OPENDDS_IDL_OPTIONS -Lc++11
  USE_EXPORT ${export}
)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenDDS::Rtps_Udp)

add_subdirectory(tests)
