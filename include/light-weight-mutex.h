/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#ifndef LIGHT_WEIGHT_MUTEX_H
#define LIGHT_WEIGHT_MUTEX_H

#ifndef INC_FREERTOS_H
    #error "include FreeRTOS.h must appear in source files before include light_weight_mutex.h"
#endif

/* *INDENT-OFF* */
#if defined( __cplusplus )
    extern "C" {
#endif
/* *INDENT-ON* */

typedef struct xLW_MUTEX
{
    uintptr_t owner;
    UBaseType_t lock_count;
    UBaseType_t uxCeilingPriority;
    ListItem_t xMutexHolderListItem; /*< List of mutexes held by the task. */
    List_t xTasksWaitingForMutex;    /*< List of tasks that are blocked waiting to take the mutex.  Stored in priority order. */
} LightWeightMutex_t;

void lightMutexInit( LightWeightMutex_t * xMutex );
void vRemoveMutexItemFromList( void * pvMutexHandle );
BaseType_t lightMutexTake( LightWeightMutex_t * xMutex,
                           TickType_t xTicksToWait );
BaseType_t lightMutexGive( LightWeightMutex_t * xMutex );

/* *INDENT-OFF* */
#if defined( __cplusplus )
    }
#endif
/* *INDENT-ON* */

#endif /* !defined( STREAM_BUFFER_H ) */
