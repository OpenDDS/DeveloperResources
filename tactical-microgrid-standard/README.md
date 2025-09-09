# Tactical Microgrid Standard (TMS) Implementation

This project implements the Tactical Microgrid Standard (MIL-STD-3071) using OpenDDS for communication between different components of a microgrid system. It provides a simulation for managing and controlling tactical microgrids with distributed power sources, loads, and distribution devices.

## Overview

The implementation consists of several key components:

- **Common Libraries**: Shared functionality for communication and device management, including handshaking, timer management, microgrid controller selection, and QoS profiles
- **Controller**: Microgrid controller that coordinates power devices and handles operator requests
- **Power Devices**: Including:
  - Sources (power generation)
  - Loads (power consumption)
  - Distribution (power routing)
- **CLI**: Command-line interface for operator interaction

## Prerequisites

- OpenDDS
- CMake (version 3.27 or higher)
- C++ compiler with C++17 support

## Building the Project

```bash
cmake -B <build_dir>
cmake --build <build_dir>
```

## Project Structure

- `cli/`: Command-line interface implementation
- `cli_idl/`: Interface definition files for CLI commands
- `common/`: Shared libraries and utilities
  - TMS data model definitions (`mil-std-3071_data_model.idl`)
  - TMS Handshaking function
  - TMS microgrid controller selection
  - TMS QoS profiles
  - Utility functions
- `controller/`: Microgrid controller implementation
- `power_devices/`: Power device implementations
  - Source devices
  - Load devices
  - Distribution devices
- `tests/`: Test suite

## Testing

Tests can be run using CTest after building the project with `-D BUILD_TESTING=TRUE`:

```bash
cd <build_dir>
ctest
```

## Configuration

These programs support the following OpenDDS configuration properties. There
are multiple ways to set these, see
[OpenDDS runtime configuration](https://opendds.readthedocs.io/en/master/devguide/run_time_configuration.html)
for more info.

- `TMS_SELECTOR_DEBUG=<boolean>`
  - Enables debug logging of the microgrid controller selection process of power devices.
  - Command line option example: `-OpenDDS-tms-selector-debug true`
- `TMS_CONTROLLER_DEBUG=<boolean>`
  - Enables debug logging of what devices the controller has learned about.
  - Command line option example: `-OpenDDS-tms-controller-debug true`

## References

- [Tactical Microgrid Standard (MIL-STD-3071)](https://quicksearch.dla.mil/qsDocDetails.aspx?ident_number=285095)
- [OpenDDS](https://github.com/OpenDDS/OpenDDS)
