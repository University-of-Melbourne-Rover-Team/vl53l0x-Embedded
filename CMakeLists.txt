cmake_minimum_required(VERSION 3.13)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(urt_vl53l0x C CXX)
pico_sdk_init()

add_executable(urt_vl53
  src/main.c
  src/vl53.c
  src/platform/vl53l0x_platform.c  
  st_api/vl53l0x_api.c
  st_api/vl53l0x_api_core.c
  st_api/vl53l0x_api_ranging.c
  st_api/vl53l0x_api_calibration.c
  st_api/vl53l0x_api_strings.c
)

target_include_directories(urt_vl53 PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/src
  ${CMAKE_CURRENT_LIST_DIR}/src/platform
  ${CMAKE_CURRENT_LIST_DIR}/st_api
)

target_link_libraries(urt_vl53 pico_stdlib hardware_i2c)

pico_enable_stdio_usb(urt_vl53 1)
pico_enable_stdio_uart(urt_vl53 0)
pico_add_extra_outputs(urt_vl53)
