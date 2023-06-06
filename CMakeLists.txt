# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(summit)

target_sources(app PRIVATE
  src/main.c
  src/wifi.c
  src/mqtt_service.c
  src/mqtt_shell.c
  src/lvgl/esp_logo.c
  src/lvgl/esp_text.c
  src/lvgl/zephyr_logo.c
)
