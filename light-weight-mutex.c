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
        vListInitialise( &( pxMutex->xTasksWaitingToReceive ) );
    }

/*-----------------------------------------------------------*/

    BaseType_t lightMutexTake( LightWeightMutex_t * pxMutex,
                               TickType_t xTicksToWait )
    {
        TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
        uintptr_t expectedOwner = 0;
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

        while( pdTRUE )
        {
            if( Atomic_CompareAndSwap_u32( &pxMutex->owner, ( uintptr_t ) currentTask, expectedOwner ) )
            {
                pxMutex->lock_count = 1;
                xReturn = pdTRUE;
                goto exit;
            }

            if( expectedOwner == ( uintptr_t ) currentTask )
            {
                pxMutex->lock_count++;
                xReturn = pdTRUE;
                goto exit;
            }

            if( xTicksToWait != portMAX_DELAY )
            {
                if( ( xTaskGetTickCount() - startTime ) >= xTicksToWait )
                {
                    xReturn = pdTRUE;
                    goto exit;
                }
            }

            vTaskPlaceOnEventList( &( pxMutex->xTasksWaitingForMutex ), xTicksToWait );
            taskYIELD();
            expectedOwner = 0;
        }

exit:
        return xReturn;
    }

/*-----------------------------------------------------------*/

    BaseType_t lightMutexGive( LightWeightMutex_t * pxMutex )
    {
        TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();

        /* Check the pxMutex pointer is not NULL. */
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

        if( Atomic_Load_u32( &pxMutex->owner ) != ( uintptr_t ) currentTask )
        {
            return pdFALSE;
        }

        pxMutex->lock_count--;

        if( pxMutex->lock_count == 0 )
        {
            uintptr_t expectedOwner = ( uintptr_t ) currentTask;

            if( !Atomic_CompareAndSwap_u32( &pxMutex->owner, 0, expectedOwner ) )
            {
                /* This should never happen if used correctly */
                configASSERT( pdFALSE );
                return pdFALSE;
            }
        }

        /* Get the new owner, if any. */
        if( listLIST_IS_EMPTY( &( pxMutex->xTasksWaitingForMutex ) ) == pdFALSE )
        {
            TaskHandle_t newOwner = NULL;
            newOwner = ( TaskHandle_t ) listGET_OWNER_OF_NEXT_ENTRY( &( pxMutex->xTasksWaitingForMutex ) );
            Atomic_Store_u32( &pxMutex->owner, ( uintptr_t ) newOwner );
            pxMutex->lock_count = 1;
            listREMOVE_HEAD( &( pxMutex->xTasksWaitingForMutex ) );
        }

        return pdTRUE;
    }

/*-----------------------------------------------------------*/

/* This entire source file will be skipped if the application is not configured
 * to include light weight mutex functionality. This #if is closed at the very
 * bottom of this file. If you want to include light weight mutexes then ensure
 * configUSE_LW_MUTEXES is set to 1 in FreeRTOSConfig.h. */
#endif /* configUSE_LW_MUTEXES == 1 */
