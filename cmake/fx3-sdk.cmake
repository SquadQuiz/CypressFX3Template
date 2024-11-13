################################################################################
# Configure CMake and setup variables for the Cypress FX3 SDK v1.3.5
################################################################################

cmake_minimum_required(VERSION 3.20)

# Check required environment variables first
if(NOT DEFINED ENV{FX3_INSTALL_PATH})
  message(FATAL_ERROR "FX3_INSTALL_PATH environment variable is not set")
endif()

if(NOT DEFINED ENV{ARMGCC_INSTALL_PATH})
  message(FATAL_ERROR "ARMGCC_INSTALL_PATH environment variable is not set")
endif()

if(NOT DEFINED ENV{ARMGCC_VERSION})
  message(FATAL_ERROR "ARMGCC_VERSION environment variable is not set")
endif()

# Basic paths setup
set(FX3_INSTALL_PATH "$ENV{FX3_INSTALL_PATH}")
set(FX3_COMPILER_PATH "$ENV{ARMGCC_INSTALL_PATH}")
set(FX3_GCC_VERSION "$ENV{ARMGCC_VERSION}")

# SDK version configuration
set(FX3_SDK_VERSION         "1.3.5")
string(REPLACE "." "_" FX3_SDK_VERSION_PATH "${FX3_SDK_VERSION}")

# Setup directory paths
set(FX3_FWLIB_DIR "${FX3_INSTALL_PATH}fw_lib/${FX3_SDK_VERSION_PATH}")
set(FX3_FW_COMMON_DIR "${FX3_INSTALL_PATH}firmware/common")
set(FX3_INCLUDE_DIR "${FX3_FWLIB_DIR}/inc")
set(FX3_ELF2IMG "${FX3_INSTALL_PATH}util/elf2img/elf2img.exe")
set(CMAKE_MAKE_PROGRAM "${FX3_COMPILER_PATH}bin/cs-make.exe")

# Set library directory based on build type
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(FX3_LIBRARY_DIR "${FX3_FWLIB_DIR}/fx3_debug")
else()
  set(FX3_LIBRARY_DIR "${FX3_FWLIB_DIR}/fx3_release")
endif()

# Setup compiler paths
set(FX3_LIBGCC_PATH "${FX3_COMPILER_PATH}lib/gcc/arm-none-eabi/${FX3_GCC_VERSION}/")
set(FX3_LIBC_PATH "${FX3_COMPILER_PATH}arm-none-eabi/lib/")

# Cache all the paths
set(FX3_INSTALL_PATH ${FX3_INSTALL_PATH}                CACHE PATH "Path to FX3 SDK")
set(FX3_INCLUDE_DIR ${FX3_INCLUDE_DIR}                  CACHE PATH "Path to FX3 header directory")
set(FX3_LIBRARY_DIR ${FX3_LIBRARY_DIR}                  CACHE PATH "FX3 Library directory")
set(FX3_FW_COMMON_DIR ${FX3_FW_COMMON_DIR}              CACHE PATH "FX3 common firmware directory")
set(FX3_LINKER_FILE "${FX3_FW_COMMON_DIR}/fx3_512k.ld"  CACHE PATH "FX3 Linker script")
set(FX3_ELF2IMG ${FX3_ELF2IMG}                          CACHE PATH "FX3 ELF to IMG converter")
set(FX3_LIBGCC_PATH ${FX3_LIBGCC_PATH}                  CACHE PATH "Path to libgcc.a")
set(FX3_LIBC_PATH ${FX3_LIBC_PATH}                      CACHE PATH "Path to libc.a")

# Cache string
set(FX3_GCC_VERSION ${FX3_GCC_VERSION}                    CACHE STRING "FX3 SDK GCC Version")
set(FX3_SDK_VERSION ${FX3_SDK_VERSION}                    CACHE STRING "FX3 SDK Version")
set(FX3_SDK_VERSION_PATH ${FX3_SDK_VERSION_PATH}          CACHE STRING "FX3 SDK Version Path")

# Perform some sanity checks on the above paths
if(NOT EXISTS "${FX3_INSTALL_PATH}")
    message(FATAL_ERROR "FX3 SDK path not found: ${FX3_INSTALL_PATH}")
endif()

if(NOT EXISTS "${FX3_INCLUDE_DIR}/cyu3os.h")
    message(FATAL_ERROR "FX3 SDK headers not found in: ${FX3_INCLUDE_DIR}")
endif()

if(NOT EXISTS "${FX3_LIBRARY_DIR}/libcyfxapi.a")
    message(FATAL_ERROR "FX3 libraries not found in: ${FX3_LIBRARY_DIR}")
endif()

if(NOT EXISTS "${FX3_FWLIB_DIR}")
  message(FATAL_ERROR "FX3 SDK: Could not find ${FX3_FWLIB_DIR}")
endif()

if(NOT EXISTS "${FX3_LINKER_FILE}")
  message(FATAL_ERROR "FX3 SDK: Could not find linker file: ${FX3_LINKER_FILE}")
endif()

if(NOT EXISTS "${FX3_ELF2IMG}")
  message(FATAL_ERROR "elf2img utility not found: ${FX3_ELF2IMG}")
endif()

if(NOT EXISTS "${FX3_LIBGCC_PATH}/libgcc.a")
  message(FATAL_ERROR "Could not find libgcc.a on: ${FX3_LIBGCC_PATH}")
endif()

if(NOT EXISTS "${FX3_LIBC_PATH}/libc.a")
  message(FATAL_ERROR "Could not find libc.a on: ${FX3_LIBC_PATH}")
endif()

# CMake toolchain settings
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH "${FX3_INSTALL_PATH}ARM GCC")

################################################################################
