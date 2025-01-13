#include "FreeRTOS.h"
#include "task.h"
#include "light-weight-mutex.h"
#include <stdatomic.h>

/* This entire source file will be skipped if the application is not configured
 * to include light weight mutex functionality. This #if is closed at the very
 * bottom of this file. If you want to include light weight mutexes then ensure
 * configUSE_LW_MUTEXES is set to 1 in FreeRTOSConfig.h. */
#if ( configUSE_LW_MUTEXES == 1 )

    void lightMutexInit( LightWeightMutex_t * xMutex )
    {
        atomic_store(&xMutex->owner, 0);
        xMutex->lock_count = 0;
    }

    /*-----------------------------------------------------------*/

    BaseType_t lightMutexTake( LightWeightMutex_t * xMutex, TickType_t xTicksToWait )
    {
        TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
        uintptr_t expectedOwner = 0;
        BaseType_t xReturn = pdFAIL;
        TickType_t startTime = xTaskGetTickCount();

        /* Check the xMutex pointer is not NULL. */
        if( xMutex == NULL )
        {
            xReturn = pdFAIL;
            goto exit;
        }

        /* Cannot block if the scheduler is suspended. */
        #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
        {
            if( ( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) && (  != 0U ) )
            {
                xReturn = pdFAIL;
                goto exit;
            }
        }
        #endif

        while (true) {
            if (atomic_compare_exchange_strong(&xMutex->owner, &expectedOwner, (uintptr_t)currentTask))
            {
                xMutex->lock_count = 1;
                xReturn = pdPASS;
                goto exit;
            }

            if (expectedOwner == (uintptr_t)currentTask)
            {
                xMutex->lock_count++;
                xReturn = pdPASS;
                goto exit;
            }

            if (blockTime != portMAX_DELAY) {
                if ((xTaskGetTickCount() - startTime) >= blockTime)
                {
                    xReturn = pdFAIL;
                    goto exit;
                }
            }

            taskYIELD();
            expectedOwner = 0;
        }
    exit:
        return xReturn;
    }

    /*-----------------------------------------------------------*/

    BaseType_t lightMutexGive( LightWeightMutex_t * xMutex )
    {
        TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();

        /* Check the xMutex pointer is not NULL. */
        if( xMutex == NULL )
        {
            xReturn = pdFALSE;
            goto exit;
        }

        /* Cannot block if the scheduler is suspended. */
        #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
        {
            if( ( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) && (  != 0U ) )
            {
                xReturn = pdFALSE;
                goto exit;
            }
        }
        #endif

        if (atomic_load(&xMutex->owner) != (uintptr_t)currentTask) {
            return pdFALSE;
        }

        xMutex->lock_count--;

        if (xMutex->lock_count == 0) {
            uintptr_t expectedOwner = (uintptr_t)currentTask;
            if (!atomic_compare_exchange_strong(&xMutex->owner, &expectedOwner, 0)) {
                // This should never happen if used correctly
                configASSERT( pdFALSE );
                return pdFALSE;
            }
        }

        return pdTRUE;
    }

    /*-----------------------------------------------------------*/

/* This entire source file will be skipped if the application is not configured
 * to include light weight mutex functionality. This #if is closed at the very
 * bottom of this file. If you want to include light weight mutexes then ensure
 * configUSE_LW_MUTEXES is set to 1 in FreeRTOSConfig.h. */
#endif /* configUSE_LW_MUTEXES == 1 */