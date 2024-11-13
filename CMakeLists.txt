cmake_minimum_required(VERSION 3.22)

# Options
option(USE_VERSION_IN_OUTPUT "Include version number in output filename" OFF)
option(USE_DATE_IN_OUTPUT "Include date in output filename" OFF)

# Set project version
set(PROJ_VER_MAJOR 1)
set(PROJ_VER_MINOR 0)
set(PROJ_VER_PATCH 0)

# Get current date
string(TIMESTAMP CURRENT_DATE "%y%m%d")

# Core project settings
project(
    CyFx3Fw
    VERSION ${PROJ_VER_MAJOR}.${PROJ_VER_MINOR}.${PROJ_VER_PATCH}
    LANGUAGES C ASM CXX
    DESCRIPTION "Cypress FX3 Firmware Project"
)

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Include toolchain file
include("cmake/fx3-toolchain.cmake")

# Build the output name based on options
set(OUTPUT_NAME ${CMAKE_PROJECT_NAME})  # Initialize with project name first

if(USE_VERSION_IN_OUTPUT)
    string(APPEND OUTPUT_NAME "_v${PROJ_VER_MAJOR}.${PROJ_VER_MINOR}.${PROJ_VER_PATCH}")
endif()
if(USE_DATE_IN_OUTPUT)
    string(APPEND OUTPUT_NAME "_${CURRENT_DATE}")
endif()

# Cache the output name
set(OUTPUT_NAME ${OUTPUT_NAME} CACHE STRING "Output filename")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

message("Project name: "        ${CMAKE_PROJECT_NAME})
message("Project version: "     ${CMAKE_PROJECT_VERSION})
message("Build type: "          ${CMAKE_BUILD_TYPE})
message("Output name: "         ${OUTPUT_NAME})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_SRC
    ${FX3_FW_COMMON_DIR}/cyfx_gcc_startup.S
    ${FX3_FW_COMMON_DIR}/cyfxtx.c
    ${SRC_DIR}/main.c
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} 
    PRIVATE
        ${PROJECT_SRC}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} 
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${FX3_COMPILER_PATH}/arm-none-eabi/include/
        ${FX3_COMPILER_PATH}/lib/gcc/arm-none-eabi/${FX3_GCC_VERSION}/include/
        ${FX3_INCLUDE_DIR}
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} 
    PRIVATE

)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${FX3_LIBRARY_DIR}/libcyfxapi.a
        ${FX3_LIBRARY_DIR}/libcyu3lpp.a
        ${FX3_LIBRARY_DIR}/libcyu3threadx.a
        ${FX3_LIBC_PATH}/libc.a
        ${FX3_LIBGCC_PATH}/libgcc.a
)

# Set output name for the executable
set_target_properties(${CMAKE_PROJECT_NAME} 
    PROPERTIES 
    OUTPUT_NAME ${OUTPUT_NAME}
    SUFFIX ".elf"
    LINK_FLAGS "-Wl,-Map=${OUTPUT_NAME}.map"
)

# Create a list to store all generated files
set(GENERATED_FILES
    ${OUTPUT_NAME}.hex
    ${OUTPUT_NAME}.img
    ${OUTPUT_NAME}.lst
    ${OUTPUT_NAME}.map
)

# Post build commands
add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}
    POST_BUILD

    COMMENT "Performing post-build steps..."

    COMMAND ${CMAKE_COMMAND} -E echo "Generating HEX file..."
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${OUTPUT_NAME}.hex

    COMMAND ${CMAKE_COMMAND} -E echo "Size information..."
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${CMAKE_PROJECT_NAME}>

    COMMAND ${CMAKE_COMMAND} -E echo "Converting ELF to IMG file..."
    COMMAND ${FX3_ELF2IMG} -i $<TARGET_FILE:${CMAKE_PROJECT_NAME}> -o ${OUTPUT_NAME}.img
)

# Debug-only commands
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(
        TARGET ${CMAKE_PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Generating assembly listing..."
        COMMAND ${CMAKE_OBJDUMP} -h -S $<TARGET_FILE:${CMAKE_PROJECT_NAME}> > ${OUTPUT_NAME}.lst
    )
endif()