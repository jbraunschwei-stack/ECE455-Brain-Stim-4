cmake_minimum_required(VERSION 3.13)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD seeed_xiao_rp2040 CACHE STRING "Board type")

# Pull in the Pico SDK (must be set before project())
if(NOT PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake")
    include(pico_sdk_import.cmake)
elseif(EXISTS "${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
    include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
else()
    message(FATAL_ERROR "Set PICO_SDK_PATH to an installed Raspberry Pi Pico SDK (tested with 2.2.0).")
endif()

project(rp2040_dds C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
option(DDS_ENABLE_LEAD_MONITOR "Enable a separately wired GPIO29 detector" OFF)

add_executable(rp2040_dds
    main.c
    spi_dac.c
    dds_tables.c
    i2c_slave.c
    dds_control.c
)

target_include_directories(rp2040_dds PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rp2040_dds
    pico_stdlib
    pico_multicore
    pico_i2c_slave
    hardware_spi
    hardware_gpio
    hardware_i2c
    hardware_watchdog
    hardware_adc
)
target_compile_definitions(rp2040_dds PRIVATE DDS_ENABLE_LEAD_MONITOR=$<BOOL:${DDS_ENABLE_LEAD_MONITOR}>)
target_compile_options(rp2040_dds PRIVATE -Wall -Wextra)
# DDS uses I2C only. USB serial is provided by the separate manual test.
pico_enable_stdio_usb(rp2040_dds 0)
pico_enable_stdio_uart(rp2040_dds 0)

# Generate UF2, bin, hex outputs
pico_add_extra_outputs(rp2040_dds)

add_executable(dac_bench_test dac_bench_test.c spi_dac.c)
target_link_libraries(dac_bench_test pico_stdlib hardware_spi)
target_compile_definitions(dac_bench_test PRIVATE DAC_SPI_BAUD_HZ=1000000U)
target_compile_options(dac_bench_test PRIVATE -Wall -Wextra)
pico_enable_stdio_usb(dac_bench_test 1)
pico_enable_stdio_uart(dac_bench_test 0)
pico_add_extra_outputs(dac_bench_test)
