# Set up cross-compiler
set (CMAKE_SYSTEM_NAME Generic)
set (CMAKE_SYSTEM_PROCESSOR arm)
set (CROSS_COMPILE arm-none-eabi)
set (CMAKE_C_COMPILER ${CROSS_COMPILE}-gcc)
set (CMAKE_SIZE ${CROSS_COMPILE}-size)

# Search for the toolchain from some pre-defined location(s)
if (NOT TOOLCHAIN_PATH)
    find_path (TOOLCHAIN_PATH
        NAMES bin/${CMAKE_C_COMPILER}
        PATHS /usr/arm-none-eabi /opt/toolchains/stm32 /opt/st/stm32cubeide_1.13.1/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.11.3.rel1.linux64_1.1.0.202305231506/tools
    )
    if (TOOLCHAIN_PATH)
        message (STATUS "Toolchain found: ${TOOLCHAIN_PATH}")
    endif ()
endif ()

# Set full path to tools if TOOLCHAIN_PATH was found
if (TOOLCHAIN_PATH)
    set (TOOLCHAIN_BIN_PREFIX ${TOOLCHAIN_PATH}/bin/)
    set (CMAKE_C_COMPILER ${TOOLCHAIN_BIN_PREFIX}/${CMAKE_C_COMPILER})
    set (CMAKE_SIZE ${TOOLCHAIN_BIN_PREFIX}/${CMAKE_SIZE})
endif ()

# Skip the compiler checks
set (CMAKE_C_COMPILER_WORKS 1)

set (CMAKE_C_FLAGS_INIT "-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -mthumb")
set (CMAKE_ASM_FLAGS_INIT "-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -mthumb")
set (CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nano.specs")
