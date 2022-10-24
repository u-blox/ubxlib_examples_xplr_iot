# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LTR303)
  zephyr_include_directories(.)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LTR303 ltr303.c)

endif()