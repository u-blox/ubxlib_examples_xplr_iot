# Copyright 2022 u-blox
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Include ubxlib
include($ENV{UBXLIB_DIR}/zephyr/ubxlib.cmake)
# Add XPLR-IOT-1 specifics, remove this for other boards
include(${CMAKE_CURRENT_LIST_DIR}/../config/xplriot1.cmake)
# Set debug compiler settings as default
if (NOT NO_DEBUG)
  list(APPEND CONF_FILE ${CMAKE_CURRENT_LIST_DIR}/debug.conf)
endif()
# And Zephyr
find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
# All example source code
file(GLOB_RECURSE APP_SOURCES "${CMAKE_SOURCE_DIR}/src/*.c")
target_sources(app PRIVATE ${APP_SOURCES})
# And common utilities
file(REAL_PATH "${CMAKE_CURRENT_LIST_DIR}/common" APP_COMMON_DIR)
file(GLOB APP_COMMONS "${APP_COMMON_DIR}/*.c")
target_sources(app PRIVATE ${APP_COMMONS})
target_include_directories(app PRIVATE ${APP_COMMON_DIR})