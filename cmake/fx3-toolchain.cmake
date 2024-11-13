include(cmake/fx3-sdk.cmake)

set(CMAKE_SYSTEM_NAME               Generic)
set(CMAKE_SYSTEM_PROCESSOR          arm)

set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER_ID GNU)
set(CMAKE_CXX_COMPILER_ID GNU)

set(TOOLCHAIN_PREFIX                ${FX3_COMPILER_PATH}/bin/arm-none-eabi-)

set(CMAKE_C_COMPILER                ${TOOLCHAIN_PREFIX}gcc.exe)
set(CMAKE_ASM_COMPILER              ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER              ${TOOLCHAIN_PREFIX}g++.exe)
set(CMAKE_LINKER                    ${TOOLCHAIN_PREFIX}ld.exe)
set(CMAKE_OBJCOPY                   ${TOOLCHAIN_PREFIX}objcopy.exe)
set(CMAKE_SIZE                      ${TOOLCHAIN_PREFIX}size.exe)

if(NOT EXISTS ${CMAKE_C_COMPILER})
  message(FATAL_ERROR "Could not find compiler: ${CMAKE_C_COMPILER}")
endif()

set(CMAKE_EXECUTABLE_SUFFIX_ASM     ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C       ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX     ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_EXE     ".exe")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Initialize flag collections
set(FX3_TARGET_FLAGS
  -mcpu=arm926ej-s      # ARM926EJ-S CPU
  -marm                 # Forces ARM (not Thumb) mode.
  -mthumb-interwork     # Allows interworking between ARM and Thumb modes.
)

set(FX3_OPT_FLAGS
  -fmessage-length=0    # Sets error message output to unlimited length
  -fsigned-char         # Treats char as signed.
  -ffunction-sections   # Places each function in separate sections
  -fdata-sections       # Places each data in separate sections
)

set(FX3_WARNING_FLAGS
  -Wall                 # Enables all common warnings.
  -Wextra               # additional warnings not covered by -Wall
  -Wno-unused-parameter # Suppresses warnings for unused function parameters
)

set(FX3_MACRO_FLAGS
  -DCYU3P_FX3=1        # FX3 specific define
  -D__CYU3P_TX__=1     # ThreadX RTOS define
)

# Create base flags list
set(BASE_C_FLAGS
  ${FX3_TARGET_FLAGS}
  ${FX3_OPT_FLAGS}
  ${FX3_WARNING_FLAGS}
  ${FX3_MACRO_FLAGS}
)

# Add debug/release flags to the list
if(CMAKE_BUILD_TYPE MATCHES Debug)
  list(APPEND BASE_C_FLAGS
    -O0                 # No optimization
    -g3                 # Maximum debug info
    -DDEBUG             # Debug build
  )
endif()
if(CMAKE_BUILD_TYPE MATCHES Release)
  list(APPEND BASE_C_FLAGS
    -O3                 # Optimize Most
    -g                  # Debug level default
  )
endif()

# Convert lists to space-separated strings
string(REPLACE ";" " " CMAKE_C_FLAGS "${BASE_C_FLAGS}")

# Set ASM flags (also using string replace)
set(FX3_ASM_FLAGS
  -x assembler-with-cpp # Process assembly as C preprocessor
  -MMD                  # Generate dependency files
  -MP                   # Add phony targets
)
string(REPLACE ";" " " ASM_FLAGS_STR "${FX3_ASM_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} ${ASM_FLAGS_STR}")

# Linker flags
set(FX3_LINKER_FLAGS
  -nostartfiles                       # Don't use standard start files
  -Xlinker --gc-sections              # Removes unused sections
  -T"${FX3_LINKER_FILE}"              # Linker script
  -Wl,--entry,CyU3PFirmwareEntry      # Entry point
  -Wl,-d                              # Display removed sections
  -Wl,--gc-sections                   # Remove unused sections
  -Wl,--no-wchar-size-warning         # Suppress wchar warnings
)

# Convert linker flags to space-separated string
string(REPLACE ";" " " LINKER_FLAGS_STR "${FX3_LINKER_FLAGS}")

# Set linker flags
set(CMAKE_C_LINK_FLAGS "${LINKER_FLAGS_STR}")
