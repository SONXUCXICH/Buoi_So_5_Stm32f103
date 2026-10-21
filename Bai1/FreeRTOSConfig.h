#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

/* Clock he thong */
#define configCPU_CLOCK_HZ                     72000000UL
#define configTICK_RATE_HZ                     1000U

/* Scheduler */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_IDLE_HOOK    0
#define configUSE_TICK_HOOK                     0
#define configMAX_PRIORITIES                   5
#define configMINIMAL_STACK_SIZE               128
#define configMAX_TASK_NAME_LEN                16
#define configUSE_16_BIT_TICKS                 0
#define configIDLE_SHOULD_YIELD                1

/* Heap */
#define configSUPPORT_DYNAMIC_ALLOCATION       1
#define configSUPPORT_STATIC_ALLOCATION        0
#define configTOTAL_HEAP_SIZE                  (8U * 1024U)

/* Cac tinh nang khong su dung */
#define configUSE_MUTEXES                      0
#define configUSE_RECURSIVE_MUTEXES            0
#define configUSE_COUNTING_SEMAPHORES          0
#define configUSE_QUEUE_SETS                   0
#define configUSE_TASK_NOTIFICATIONS           1
#define configUSE_TIMERS                       0
#define configUSE_EVENT_GROUPS                 0

/* Debug */
#define configUSE_TRACE_FACILITY               0
#define configCHECK_FOR_STACK_OVERFLOW         0
#define configUSE_MALLOC_FAILED_HOOK           0

/* Interrupt priority cho Cortex-M3 */
#define configPRIO_BITS                        4

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY       15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY  5

#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* Ket noi interrupt handler voi FreeRTOS */
#define vPortSVCHandler        SVC_Handler
#define xPortPendSVHandler     PendSV_Handler
#define xPortSysTickHandler    SysTick_Handler

/* API */
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_vTaskDelete                 1
#define INCLUDE_vTaskSuspend                1
#define INCLUDE_xTaskGetSchedulerState      1
#define INCLUDE_xTaskGetCurrentTaskHandle   1

/* Kiem tra loi */
#define configASSERT(x) \
    do { \
        if ((x) == 0) { \
            taskDISABLE_INTERRUPTS(); \
            for (;;) { \
            } \
        } \
    } while (0)

#endif