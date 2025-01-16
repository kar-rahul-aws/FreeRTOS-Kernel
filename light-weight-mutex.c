#include "FreeRTOS.h"
#include "atomic.h"
#include "task.h"
#include "light-weight-mutex.h"

/* This entire source file will be skipped if the application is not configured
 * to include light weight mutex functionality. This #if is closed at the very
 * bottom of this file. If you want to include light weight mutexes then ensure
 * configUSE_LW_MUTEXES is set to 1 in FreeRTOSConfig.h. */
#if ( configUSE_LW_MUTEXES == 1 )

    void lightMutexInit( LightWeightMutex_t * pxMutex )
    {
        Atomic_Store_u32( &pxMutex->owner, 0 );
        pxMutex->lock_count = 0;
        vListInitialise( &( pxMutex->xTasksWaitingForMutex ) );
    }

/*-----------------------------------------------------------*/

    BaseType_t lightMutexTake( LightWeightMutex_t * pxMutex,
                               TickType_t xTicksToWait )
    {
        TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
        BaseType_t xReturn = pdFALSE;
        TickType_t startTime = xTaskGetTickCount();

        /* Check the pxMutex pointer is not NULL. */
        if( pxMutex == NULL )
        {
            xReturn = pdFALSE;
            goto exit;
        }

        /* Cannot block if the scheduler is suspended. */
        #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
        {
            if( ( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) && ( xTicksToWait != 0U ) )
            {
                xReturn = pdFALSE;
                goto exit;
            }
        }
        #endif

        for( ; ; )
        {
            taskENTER_CRITICAL();
            {
                if( ( pxMutex->lock_count == 0U ) && ( Atomic_CompareAndSwap_u32( &pxMutex->owner, ( uintptr_t ) currentTask, 0 ) ) )
                {
                    pxMutex->lock_count = 1;
                    xReturn = pdTRUE;
                    taskEXIT_CRITICAL();
                    goto exit;
                }

                if( ( uintptr_t ) Atomic_Load_u32( &pxMutex->owner ) == ( uintptr_t ) currentTask )
                {
                    pxMutex->lock_count++;
                    xReturn = pdTRUE;
                    taskEXIT_CRITICAL();
                    goto exit;
                }

                if( xTicksToWait != portMAX_DELAY )
                {
                    if( ( xTaskGetTickCount() - startTime ) >= xTicksToWait )
                    {
                        xReturn = pdFALSE;
                        taskEXIT_CRITICAL();
                        goto exit;
                    }
                }

                vTaskPlaceOnEventList( &( pxMutex->xTasksWaitingForMutex ), xTicksToWait );
            }
            taskEXIT_CRITICAL();
            taskYIELD();
        }

exit:
        return xReturn;
    }

/*-----------------------------------------------------------*/

    BaseType_t lightMutexGive( LightWeightMutex_t * pxMutex )
    {
        TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();

        /* Check the pxMutex pointer is not NULL and the mutex has already been taken earlier. */
        if( ( pxMutex == NULL ) || ( pxMutex->lock_count == 0U ) )
        {
            return pdFALSE;
        }

        /* Cannot block if the scheduler is suspended. */
        #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) )
        {
            if( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED )
            {
                return pdFALSE;
            }
        }
        #endif

        taskENTER_CRITICAL();
        {
            if( ( uintptr_t ) Atomic_Load_u32( &pxMutex->owner ) != ( uintptr_t ) currentTask )
            {
                taskEXIT_CRITICAL();
                return pdFALSE;
            }

            pxMutex->lock_count--;

            if( pxMutex->lock_count == 0U )
            {
                if( !Atomic_CompareAndSwap_u32( &pxMutex->owner, 0, ( uintptr_t ) currentTask ) )
                {
                    /* This should never happen if used correctly */
                    configASSERT( pdFALSE );
                    return pdFALSE;
                }

                /* Get the new owner, if any. */
                prvAssignLWMutexOwner( pxMutex );
            }
        }
        taskEXIT_CRITICAL();

        return pdTRUE;
    }

/*-----------------------------------------------------------*/

/* This entire source file will be skipped if the application is not configured
 * to include light weight mutex functionality. This #if is closed at the very
 * bottom of this file. If you want to include light weight mutexes then ensure
 * configUSE_LW_MUTEXES is set to 1 in FreeRTOSConfig.h. */
#endif /* configUSE_LW_MUTEXES == 1 */
