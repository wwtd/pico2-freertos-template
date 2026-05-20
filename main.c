#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#include "inspector.h"

/* ── Experiment 4: SMP Scheduler Behavior ──────────
 *
 * FLOATING=1: 4 tasks at same priority, unbound (both cores)
 * FLOATING=0: 4 tasks, 2 pinned to core0, 2 to core1
 *
 * Timeline: 0 = running on core 0,  1 = running on core 1
 *           - = ready,  . = blocked
 */

#define FLOATING    1

#define PRIO_TASK   2
#define PRIO_INSP   3
#define NTASKS      4

static char task_names[NTASKS][8];

static void vBusyTask(void *param) {
    int id = (int)(uintptr_t)param;
    vTaskDelay(pdMS_TO_TICKS(id * 50));
    TickType_t xLastWake = xTaskGetTickCount();
    for (;;) {
        TickType_t start = xTaskGetTickCount();
        while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(200));
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(300));
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("stack overflow: %s\n", pcTaskName);
    __asm volatile("bkpt #0");
    for (;;);
}

int main(void) {
    stdio_init_all();
    sleep_ms(1500);

#if FLOATING
    printf("\n=== Exp 4: SMP Unbound Scheduling ===\n");
    printf("4 tasks float between both cores\n");
    printf("Look for 0 and 1 mixed in each task's trace\n");
#else
    printf("\n=== Exp 4: SMP Pinned Scheduling ===\n");
    printf("tasks 0,1 on core0  |  tasks 2,3 on core1\n");
#endif

    for (int i = 0; i < NTASKS; i++) {
        snprintf(task_names[i], sizeof(task_names[i]), "task_%d", i);

#if FLOATING
        UBaseType_t affinity = (1 << 0) | (1 << 1);
#else
        UBaseType_t affinity = (i < 2) ? (1 << 0) : (1 << 1);
#endif

        TaskHandle_t h;
        xTaskCreateAffinitySet(vBusyTask, task_names[i],
                               configMINIMAL_STACK_SIZE, NULL,
                               PRIO_TASK, affinity, &h);
    }

    vStartInspector(PRIO_INSP);

    vTaskStartScheduler();
    for (;;);
}
