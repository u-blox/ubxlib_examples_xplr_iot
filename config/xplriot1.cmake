
find_package(Git QUIET REQUIRED)
execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags
    WORKING_DIRECTORY "$ENV{ZEPHYR_BASE}"
    OUTPUT_VARIABLE ZEPHYR_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
message("Zephyr version: ${ZEPHYR_VERSION}")
if (${ZEPHYR_VERSION} STRGREATER "v3")
  list(APPEND DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/xplriot1.overlay)
else()
  list(APPEND DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/xplriot1_v2.overlay)
endif()
list(APPEND UBXLIB_SRC ${CMAKE_CURRENT_LIST_DIR}/xplriot1.c)
if ($ENV{USE_BOOTLOADER})
  list(APPEND PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/pm_static.yml)
endif()
if (NO_SENSORS)
  list(APPEND DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/no_i2c.overlay)
else()
  list(APPEND CONF_FILE ${CMAKE_CURRENT_LIST_DIR}/sensors.conf)
endif()

if (EXT_FS)
  list(APPEND CONF_FILE ${CMAKE_CURRENT_LIST_DIR}/ext_fs.conf)
  list(APPEND DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/ext_fs.overlay)
  if (CONFIG_BOARD_ENABLE_CPUNET AND NOT $ENV{USE_BOOTLOADER})
    # Explicit definition of littlefs external flash partition needed when multi image build
    list(APPEND PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/ext_fs.yml)
  endif()
endif()
