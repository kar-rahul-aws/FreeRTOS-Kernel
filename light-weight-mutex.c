#include "FreeRTOS.h"
#include "task.h"
#include "light-weight-mutex.h"

/* This entire source file will be skipped if the application is not configured
 * to include light weight mutex functionality. This #if is closed at the very
 * bottom of this file. If you want to include light weight mutexes then ensure
 * configUSE_LW_MUTEXES is set to 1 in FreeRTOSConfig.h. */
#if ( configUSE_LW_MUTEXES == 1 )

    void lightMutexInit( LightWeightMutex_t * pxMutex, UBaseType_t uxCeilingPriority )
    {
        pxMutex->owner = 0U;
        pxMutex->lock_count = 0U;
        pxMutex->uxCeilingPriority = uxCeilingPriority;
        vListInitialise( &( pxMutex->xTasksWaitingForMutex ) );
        vListInitialiseItem( &( pxMutex->xMutexHolderListItem ) );
        listSET_LIST_ITEM_OWNER( &( pxMutex->xMutexHolderListItem ), pxMutex );
        listSET_LIST_ITEM_VALUE( &( pxMutex->xMutexHolderListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxMutex->uxCeilingPriority );
    }

/*-----------------------------------------------------------*/

    void vRemoveMutexItemFromList( void * pvMutexHandle )
    {
        configASSERT( pvMutexHandle != NULL );
        LightWeightMutex_t * pxMutex = ( LightWeightMutex_t * ) pvMutexHandle;
        listREMOVE_ITEM( &( pxMutex->xMutexHolderListItem ) );
    }

/*-----------------------------------------------------------*/

    BaseType_t lightMutexTake( LightWeightMutex_t * pxMutex,
                               TickType_t xTicksToWait )
    {
        TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
        BaseType_t xReturn = pdFALSE;
        BaseType_t xInheritanceOccurred = pdFALSE;
        TickType_t startTime = xTaskGetTickCount();

        /* Check the pxMutex pointer is not NULL. */
        configASSERT( ( pxMutex != NULL ) && ( xTicksToWait != 0U ) );

        for( ; ; )
        {
            taskENTER_CRITICAL();
            {
                if( pxMutex->lock_count == 0U )
                {
                    /*Atomic_Store_u32( &pxMutex->owner, ( uintptr_t ) currentTask ); */
                    pxMutex->owner = ( uintptr_t ) currentTask;
                    pxMutex->lock_count = 1;
                    if( uxTaskPriorityGet( currentTask ) < pxMutex->uxCeilingPriority )
                    {
                        vInsertMutexToHolderList( currentTask, &( pxMutex->xMutexHolderListItem ) );
                        xInheritanceOccurred = xTaskCeilingPriorityInherit( pxMutex->uxCeilingPriority );
                    }
                    xReturn = pdTRUE;
                    taskEXIT_CRITICAL();
                    break;
                }

                /*if( ( uintptr_t ) Atomic_Load_u32( &pxMutex->owner ) == ( uintptr_t ) currentTask ) */
                if( pxMutex->owner == ( uintptr_t ) currentTask )
                {
                    pxMutex->lock_count++;
                    xReturn = pdTRUE;
                    taskEXIT_CRITICAL();
                    break;
                }

                if( xTicksToWait != portMAX_DELAY )
                {
                    /* Timed out */
                    if( ( xTaskGetTickCount() - startTime ) >= xTicksToWait )
                    {
                        /* Get the ceiling priority of next mutex held.
                         * If it not there set to base priority.
                         */
                        LightWeightMutex_t * pxNextMutex = pvRemoveMutexToHolderList( ( void * const ) pxMutex );

                        if( pxNextMutex != NULL )
                        {
                            xTaskCeilingPriorityDisInherit( pxNextMutex->uxCeilingPriority );
                        }
                        else
                        {
                            xTaskCeilingPriorityDisInheritToBasePrio();
                        }

                        xReturn = pdFALSE;
                        taskEXIT_CRITICAL();
                        break;
                    }
                }

                vTaskPlaceOnEventList( &( pxMutex->xTasksWaitingForMutex ), xTicksToWait );
                taskYIELD();
            }
            taskEXIT_CRITICAL();
        }

        return xReturn;
    }

/*-----------------------------------------------------------*/

    BaseType_t lightMutexGive( LightWeightMutex_t * pxMutex )
    {
        /* Check the pxMutex pointer is not NULL and the mutex has already been taken earlier. */
        BaseType_t xReturn = pdTRUE;

        configASSERT( ( pxMutex != NULL ) || ( pxMutex->lock_count != 0U ) );

        /*if( ( uintptr_t ) Atomic_Load_u32( &pxMutex->owner ) != ( uintptr_t ) xTaskGetCurrentTaskHandle() ) */
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

                    /*Atomic_Store_u32( &pxMutex->owner, ( uintptr_t ) 0U ); */
                    pxMutex->owner = ( uintptr_t ) 0U;
                    /* The mutex is no longer being held. */
                    if( uxTaskPriorityGet( ( TaskHandle_t ) pxMutex->owner ) == pxMutex->uxCeilingPriority )
                    {
                        LightWeightMutex_t * pxNextMutex = pvRemoveMutexToHolderList( ( void * const ) pxMutex );

                        if( pxNextMutex != NULL )
                        {
                            xTaskCeilingPriorityDisInherit( pxNextMutex->uxCeilingPriority );
                        }
                        else
                        {
                            xTaskCeilingPriorityDisInheritToBasePrio();
                        }
                    }
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
