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

/*
 *
 * A simple demo application showing how to set up
 * and use a the file system for XPLR-IOT-1 using
 * LittleFS in the external flash.
 *
 */

#include <string.h>
#include <stdio.h>

#include "ext_fs.h"

void showBootCount(void)
{
    const char *path = extFsPath("boot_count");
    struct fs_file_t file;
    fs_file_t_init(&file);
    if (fs_open(&file, path, FS_O_CREATE | FS_O_RDWR) < 0) {
        printf("Failed to open boot count file\n");
        return;
    }
    uint32_t bootCnt = 0;
    fs_read(&file, &bootCnt, sizeof(bootCnt));
    printf("Boot count: %u\n", bootCnt);
    fs_seek(&file, 0, FS_SEEK_SET);
    bootCnt++;
    fs_write(&file, &bootCnt, sizeof(bootCnt));
    fs_close(&file);
}

void creatOneFile()
{
    struct fs_file_t file;
    fs_file_t_init(&file);
    const char *path = extFsPath("hello.txt");
    const char *line = "Hello world\n";
    if (fs_open(&file, path, FS_O_CREATE | FS_O_WRITE) == 0) {
        fs_write(&file, line, strlen(line));
        fs_close(&file);
    }
}

void main()
{
    if (extFsInit()) {
        creatOneFile();
        extFSList();
        showBootCount();
    } else {
        printf("Failed to mount the file system\n");
    }

}