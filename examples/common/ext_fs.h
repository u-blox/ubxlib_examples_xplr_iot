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

#include <fs/fs.h>
#include <kernel.h>

/**
 * Initiate little_fs on the external flash memory of the XPLR-IOT-1.
 * @return  Success or failure.
 */
bool extFsInit();

/**
 * Get the file system mount point.
 * @return  Pointer to the mount point structure.
 */
struct fs_mount_t *extFsMountPoint();

/**
 * Get the full file system path including mount point name.
 * @return  Pointer to the full path.
 */
const char *extFsPath(const char *fileName);

/**
 * Get the size of the free space on the file system in kB.
 * @return Actual free size or possible negative error code
 */
int32_t extFsFree();

/**
 * Check if a file exists
 * @param   filePath  Complete file name path.
 * @return            True if the file exists.
 */
bool extFsFileExists(const char *fileName);

/**
 * Get the size of file in bytes
 * @param   filePath  Complete file name path
 * @param   size      Place to put the size
 * @return            True if the file exists
 *                    and size could be read.
 */
bool extFsFileSize(const char *fileName, size_t *size);

/**
 * Print a simple listing of the file system contents
 * to the console.
 */
void extFSList();