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
 * Logging functions
 *
 */

#include <stdarg.h>
#include <time.h>

#include "common.h"
#include "log.h"
#include "ext_fs.h"

#define MUTEX_LOCK if (pLogMutex != NULL) uPortMutexLock(pLogMutex)
#define MUTEX_UNLOCK if (pLogMutex != NULL) uPortMutexUnlock(pLogMutex)

#define LOGBUFF1SIZE 100
#define LOGBUFF2SIZE 200
static char buff1[LOGBUFF1SIZE];
static char buff2[LOGBUFF2SIZE];

static struct fs_file_t logFile;
static bool logFileOpen = false;

static uPortMutexHandle_t pLogMutex = NULL;

/// The unix network time, which is retrieved after first registration
int64_t unixNetworkTime = 0;

int openFile(const char *filename, struct fs_file_t *file)
{
    fs_file_t_init(file);
    const char *path = extFsPath(filename);
    return fs_open(file, path, FS_O_CREATE | FS_O_RDWR);
}

/// @brief Writes a log message to the terminal and the log file
/// @param log The log, which can contain string formating
/// @param  ... The variables for the string format
void writeLog(const char *log, ...)
{
    // writeLog("The %s value is %d", "rssi", 1234)
    // will log "<time>: The rssi value is 1234"

    MUTEX_LOCK;

    // construct the log format, and include the ticks time
    int32_t ticks = uPortGetTickTimeMs();

    // if we have the network time set use this
    if (unixNetworkTime > 0) {
        time_t tmTime = unixNetworkTime + (ticks/1000);
        struct tm *time = gmtime(&tmTime);
        snprintf(buff1, LOGBUFF1SIZE, "%02d:%02d:%02d: %s\n",
                                    time->tm_hour,
                                    time->tm_min,
                                    time->tm_sec,
                                    log);
    } else {
        snprintf(buff1, LOGBUFF1SIZE, "%d: %s\n", ticks, log);
    }

    // now construct the application's arguments into the log string
    va_list arglist;
    va_start(arglist, buff1);
    vsnprintf(buff2, LOGBUFF2SIZE, buff1, arglist);
    va_end(arglist);

    printf("%s", buff2);
    if (logFileOpen)
        fs_write(&logFile, buff2, strlen(buff2));

    MUTEX_UNLOCK;
}

/// @brief Close the log file
void closeLog(void)
{
    if (!logFileOpen)
        return;

    MUTEX_LOCK;

    logFileOpen = false;
    fs_close(&logFile);

    MUTEX_UNLOCK;

    printf("Log file is closed.\n");
}

#define FILE_READ_BUFFER 100
void displayLogFile(void)
{
    char buffer[FILE_READ_BUFFER];
    int count;

    if (!logFileOpen) {
        printf("Opening log file failed, cannot display log.");
        return;
    }

    printf("\n********************************************************\n");
    printf("*** LOG START ******************************************\n");
    printf("********************************************************\n");
    while((count = fs_read(&logFile, buffer, FILE_READ_BUFFER)) > 0)
        printf("%.*s", count, buffer);
    printf("\n********************************************************\n");
    printf("*** LOG END ********************************************\n");
    printf("********************************************************\n");
}

void deleteFile(const char *pFilename)
{
    const char *path = extFsPath(pFilename);
    if (fs_unlink(path) == 0)
        printf("Deleted file: %s\n", pFilename);
    else
        printf("Failed to delete file: %s\n", pFilename);
}

void startLogging(const char *pFilename) {
    int32_t errorCode = uPortMutexCreate(&pLogMutex);
    if (errorCode == 0) {
        int result = openFile(pFilename, &logFile);
        if (result == 0) {
            printf("File logging enabled\n");
            logFileOpen = true;
        } else {
            printf("* Failed to open log file: %d\n", result);
        }
    } else {
        printf("* Failed to create the log mutex: %d\n", errorCode);
        printf("Logging to the file will not be available.\n");
        pLogMutex = NULL;
    }
}