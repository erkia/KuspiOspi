cmake_minimum_required (VERSION 3.3.22 FATAL_ERROR)

get_filename_component (PROJECT_ROOT_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)

# Set up C project
project (KuspiOspi C ASM)

# Set up C flags
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

# Build-type-specific flags
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g3 -ggdb -DBUILD_DEBUG")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os -DBUILD_RELEASE")

# Set up linker flags
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -Xlinker -Map=output.map")

add_executable(KuspiOspi.elf
    bsp/CMSIS/stm32l4/Source/Templates/gcc/startup_stm32l4q5xx.s
    bsp/CMSIS/stm32l4/Source/Templates/system_stm32l4xx.c
    bsp/hal/stm32l4/Src/stm32l4xx_hal.c
    bsp/hal/stm32l4/Src/stm32l4xx_hal_cortex.c
    bsp/hal/stm32l4/Src/stm32l4xx_hal_dma.c
    bsp/hal/stm32l4/Src/stm32l4xx_hal_gpio.c
    bsp/hal/stm32l4/Src/stm32l4xx_hal_ospi.c
    bsp/hal/stm32l4/Src/stm32l4xx_hal_pwr.c
    bsp/hal/stm32l4/Src/stm32l4xx_hal_pwr_ex.c
    bsp/hal/stm32l4/Src/stm32l4xx_hal_rcc.c
    bsp/hal/stm32l4/Src/stm32l4xx_hal_rcc_ex.c
    bsp/hal/stm32l4/Src/stm32l4xx_hal_uart.c
    bsp/hal/stm32l4/Src/stm32l4xx_hal_uart_ex.c
    src/main.c
    src/syscalls.c
)

set_target_properties (KuspiOspi.elf PROPERTIES COMPILE_FLAGS "-DUSE_HAL_DRIVER -DSTM32 -DSTM32L4 -DSTM32L4Q5xx")

target_include_directories(KuspiOspi.elf PRIVATE
    bsp/CMSIS/v5/CMSIS/Core/Include
    bsp/CMSIS/stm32l4/Include
    bsp/hal/stm32l4/Inc
    src
)

target_link_options(KuspiOspi.elf PRIVATE
    "SHELL:-L ${PROJECT_ROOT_DIR}/bsp"
    "SHELL:-T stm32l4q5.ld"
)
