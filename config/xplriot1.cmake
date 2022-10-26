
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
if (DEFINED ENV{USE_BL})
  message("--- Bootloader will be required for this application")
  list(APPEND PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/pm_static.yml)
else()
  message("--- Bootloader will not be used")
endif()
if (NO_SENSORS)
  list(APPEND DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/no_i2c.overlay)
else()
  list(APPEND CONF_FILE ${CMAKE_CURRENT_LIST_DIR}/sensors.conf)
endif()

if (EXT_FS)
  list(APPEND CONF_FILE ${CMAKE_CURRENT_LIST_DIR}/ext_fs.conf)
  list(APPEND DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/ext_fs.overlay)
endif()
