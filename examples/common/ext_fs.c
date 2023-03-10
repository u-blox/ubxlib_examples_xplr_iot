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

#include <stdio.h>

#include "ext_fs.h"

#define PARTITION_NODE DT_NODELABEL(lfs)
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
struct fs_mount_t *gMountPoint;

bool extFsInit()
{
    gMountPoint = &FS_FSTAB_ENTRY(PARTITION_NODE);
#ifndef NO_PM_FIX
    // Fix for error in partition manager
    gMountPoint->storage_dev = 0;
#endif
    return fs_mount(gMountPoint) == 0;
}

struct fs_mount_t *extFsMountPoint()
{
    return gMountPoint;
}

const char *extFsPath(const char *fileName)
{
    static char path[100];
    snprintf(path, sizeof(path), "%s/%s", gMountPoint->mnt_point, fileName);
    return path;
}

int32_t extFsFree()
{
    int32_t errorOrSize;
    struct fs_statvfs sbuf;
    errorOrSize = fs_statvfs(gMountPoint->mnt_point, &sbuf);
    if (errorOrSize == 0) {
        errorOrSize = sbuf.f_frsize * sbuf.f_bfree / 1024;
    }
    return errorOrSize;
}

bool extFsFileExists(const char *fileName)
{
    static struct fs_dirent dirent;
    return fs_stat(fileName, &dirent) == 0;
}

bool extFsFileSize(const char *fileName, size_t *size)
{
    static struct fs_dirent dirent;
    bool ok = fs_stat(fileName, &dirent) == 0;
    *size = ok ? dirent.size : 0;
    return ok;
}

void extFSList()
{
    struct fs_dir_t dirp;
    static struct fs_dirent entry;

    struct fs_statvfs sbuf;
    if (fs_statvfs(extFsMountPoint()->mnt_point, &sbuf) == 0) {
        printf("\n");
        printf("File system size: %4lu kB\n", sbuf.f_frsize * sbuf.f_blocks / 1024);
        printf("Free space:       %4lu kB\n", sbuf.f_frsize * sbuf.f_bfree / 1024);
    }
    fs_dir_t_init(&dirp);
    if (fs_opendir(&dirp, extFsMountPoint()->mnt_point) != 0) {
        return;
    }
    printf("\nDirectory listing:\n");
    printf("--------------------------------\n");
    while (fs_readdir(&dirp, &entry) == 0 && entry.name[0] != 0) {
        const char *path = extFsPath(entry.name);
        static struct fs_dirent dirent;
        if (fs_stat(path, &dirent) != 0) {
        }
        printf("%-25s %6u\n", path, dirent.size);
    }
    fs_closedir(&dirp);
    printf("--------------------------------\n");
}
