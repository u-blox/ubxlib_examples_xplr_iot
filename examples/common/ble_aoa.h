/*
 * Copyright 2022 u-blox
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdbool.h>

/** Initiate BLE for angle of arrival advertisements
 * @param pBleId String to receive the 12 character BLE id (mac).
 *               Can be set to NULL.
 * @return       Success or failure.
 */
bool bleAoaInit(char *pBleId);

/** Start or stop angle of arrival advertisements
 * @param min_ms Minimum advertisement time in milliseconds.
 * @param max_ms Maximum advertisement time in milliseconds.
 * @param on     Start or stop.
 * @return       Success or failure.
 */
bool bleAoaAdvertise(uint16_t min_ms, uint16_t max_ms, bool on);

/** Set or update the periodic advertising data.
 * Advertising must be initialized before calling.
 * @param data  Advertising data.
 * @param len   Advertising data length.
 */
bool bleAoaSetAdvData(const uint8_t *data, uint8_t len);