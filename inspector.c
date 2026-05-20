#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#define INSPECTOR_DELAY_MS  2000
#define MAX_TASKS           16

static const char *task_state_name(eTaskState state) {
    switch (state) {
    case eRunning:   return "RUN";
    case eReady:     return "RDY";
    case eBlocked:   return "BLK";
    case eSuspended: return "SUS";
    case eDeleted:   return "DEL";
    default:         return "???";
    }
}

static void print_dash(size_t n) {
    for (size_t i = 0; i < n; i++) putchar('-');
}

static void print_header(void) {
    printf(" %-12s %-5s %-4s %-6s %-6s %s\n",
           "Task", "State", "Prio", "Affin", "StkRm", "Handle");
    print_dash(12 + 1 + 5 + 1 + 4 + 1 + 6 + 1 + 6 + 1 + 10);
    putchar('\n');
}

static void print_row(const TaskStatus_t *s) {
#if (configUSE_CORE_AFFINITY == 1) && (configNUMBER_OF_CORES > 1)
    char affinity[8];
    if (s->uxCoreAffinityMask == 0xFF) {
        snprintf(affinity, sizeof(affinity), "any");
    } else if (s->uxCoreAffinityMask == (1 << 0)) {
        snprintf(affinity, sizeof(affinity), "core0");
    } else if (s->uxCoreAffinityMask == (1 << 1)) {
        snprintf(affinity, sizeof(affinity), "core1");
    } else {
        snprintf(affinity, sizeof(affinity), "both");
    }
#else
    const char *affinity = "—";
#endif

    unsigned stack_words = s->usStackHighWaterMark;
    if (stack_words > 0xFFFF) stack_words = 0;

    printf(" %-12s %-5s %-4u %-6s %-6u 0x%08lX\n",
           s->pcTaskName,
           task_state_name(s->eCurrentState),
           (unsigned)s->uxCurrentPriority,
           affinity,
           stack_words,
           (unsigned long)(uintptr_t)s->xHandle);
}

static void vInspectTask(__unused void *params) {
    static TaskStatus_t tasks[MAX_TASKS];

    for (;;) {
        UBaseType_t count = uxTaskGetNumberOfTasks();
        if (count > MAX_TASKS) count = MAX_TASKS;
        count = uxTaskGetSystemState(tasks, count, NULL);

        printf("\n--- FreeRTOS Task List @ tick %lu ---\n",
               (unsigned long)xTaskGetTickCount());
        printf("  %u task(s)\n", (unsigned)count);
        print_header();
        for (UBaseType_t i = 0; i < count; i++) {
            print_row(&tasks[i]);
        }
        putchar('\n');

        vTaskDelay(pdMS_TO_TICKS(INSPECTOR_DELAY_MS));
    }
}

void vStartInspector(UBaseType_t uxPriority) {
    TaskHandle_t h;
    xTaskCreateAffinitySet(vInspectTask, "inspector",
                           configMINIMAL_STACK_SIZE * 2,
                           NULL, uxPriority,
                           (1 << 0) | (1 << 1), &h);
    (void)h;
}
