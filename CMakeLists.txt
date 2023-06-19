# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

target_sources(app PRIVATE
  src/wifi_service.c
  src/mqtt.c
  src/mqtt_service.c
#   src/lvgl/esp_logo.c
#   src/lvgl/esp_text.c
  src/lvgl/zephyr_logo.c
  src/main.c
  src/display.c
)

if (CONFIG_ESP32_NETWORK_CORE)
set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/remote-prefix/src/remote-build/zephyr)

set_source_files_properties(${REMOTE_ZEPHYR_DIR}/esp32_net_firmware.c PROPERTIES GENERATED TRUE)
target_sources(app PRIVATE src/main.c ${REMOTE_ZEPHYR_DIR}/esp32_net_firmware.c)

include(ExternalProject)

ExternalProject_Add(
	remote
	SOURCE_DIR ${APPLICATION_SOURCE_DIR}/remote
	INSTALL_COMMAND ""
	CMAKE_CACHE_ARGS -DBOARD:STRING=esp32s3_net
	BUILD_BYPRODUCTS "${REMOTE_ZEPHYR_DIR}/${KERNEL_BIN_NAME}"
	BUILD_ALWAYS True
)

add_dependencies(app remote)

endif()