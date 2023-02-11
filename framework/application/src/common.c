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
 * Common utility functions
 *
 */

#include "common.h"

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Checks if the mutex is locked
/// @param mutex the mutex of the running task
/// @return True if the mutex is locked
bool isTaskRunning(const uPortMutexHandle_t mutex)
{
    if (mutex == NULL)
        return false;

    int32_t result = uPortMutexTryLock(mutex, 0);
    if (result != 0) {
        return true;
    } else {
        if (uPortMutexUnlock(mutex) != 0) {
            printf("Failed to release mutex from lock check!!!");
            // now you've done it. You've tested the mutex lock
            // by locking it and now can't unlock it!!
            return true;
        }
    }

    return false;
}

/// @brief Duplicates a string via malloc - remember to free()!
/// @param src the source string
/// @returns either the duplicates string pointer, or NULL
char *uStrDup(const char *src)
{
    size_t len = strlen(src) + 1;  // String plus '\0'
    char *dst = pUPortMalloc(len); // Allocate space
    if(dst != NULL)
    {
        memcpy (dst, src, len); // Copy the block
    }

    // Return the new duplicate string
    return dst;
}
