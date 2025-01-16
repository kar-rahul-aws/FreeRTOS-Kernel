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
        configASSERT( ( pxMutex != NULL ) || ( xTicksToWait != 0U ) );

        taskENTER_CRITICAL();
        {
            if( pxMutex->lock_count == 0U )
            {
                //Atomic_Store_u32( &pxMutex->owner, ( uintptr_t ) currentTask );
                pxMutex->owner = ( uintptr_t ) currentTask;
                pxMutex->lock_count = 1;
                xReturn = pdTRUE;
                taskEXIT_CRITICAL();
                goto exit;
            }

            //if( ( uintptr_t ) Atomic_Load_u32( &pxMutex->owner ) == ( uintptr_t ) currentTask )
            if( pxMutex->owner == ( uintptr_t ) currentTask )
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

exit:
        return xReturn;
    }

/*-----------------------------------------------------------*/

    BaseType_t lightMutexGive( LightWeightMutex_t * pxMutex )
    {
        /* Check the pxMutex pointer is not NULL and the mutex has already been taken earlier. */
        BaseType_t xReturn = pdTRUE;

        configASSERT( ( pxMutex != NULL ) || ( pxMutex->lock_count != 0U ) ) ;

            //if( ( uintptr_t ) Atomic_Load_u32( &pxMutex->owner ) != ( uintptr_t ) xTaskGetCurrentTaskHandle() )
            if( pxMutex->owner != ( uintptr_t ) xTaskGetCurrentTaskHandle() )
            {
                xReturn = pdFALSE;
            }
            else
            {
                taskENTER_CRITICAL();
                {
                    pxMutex->lock_count--;

                    if( pxMutex->lock_count == 0U )
                    {
                        /* Update the current owner of the mutex to 0. */

                        //Atomic_Store_u32( &pxMutex->owner, ( uintptr_t ) 0U );
                        pxMutex->owner = ( uintptr_t ) 0U;
                        xReturn = pdTRUE;
                        /* Get the new owner, if any. */
                        prvAssignLWMutexOwner( pxMutex );
                    }
                }
                taskEXIT_CRITICAL();
            }

        return xReturn;
    }

/*-----------------------------------------------------------*/

/* This entire source file will be skipped if the application is not configured
 * to include light weight mutex functionality. This #if is closed at the very
 * bottom of this file. If you want to include light weight mutexes then ensure
 * configUSE_LW_MUTEXES is set to 1 in FreeRTOSConfig.h. */
#endif /* configUSE_LW_MUTEXES == 1 */
