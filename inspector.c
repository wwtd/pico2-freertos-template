#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#define MAX_TASKS    16
#define TIMELINE_W   72

/* ── timeline ring buffer ────────────────────────── */

static struct {
    TaskHandle_t handle;
    char         name[12 + 1];
    char         trace[TIMELINE_W];    /* 0 = oldest, TIMELINE_W-1 = newest */
} tl_tracks[MAX_TASKS];

static int tl_count = 0;

static int tl_find(TaskHandle_t h) {
    for (int i = 0; i < tl_count; i++)
        if (tl_tracks[i].handle == h) return i;
    return -1;
}

static int tl_find_or_add(TaskHandle_t h, const char *name) {
    int i = tl_find(h);
    if (i >= 0) return i;
    if (tl_count >= MAX_TASKS) return -1;
    i = tl_count++;
    tl_tracks[i].handle = h;
    snprintf(tl_tracks[i].name, sizeof(tl_tracks[i].name), "%s", name);
    memset(tl_tracks[i].trace, ' ', TIMELINE_W);
    return i;
}

/* ── snapshot: called every SAMPLE_MS ────────────── */

#define SAMPLE_MS  100

static void shift_and_sample(void) {
    static TaskStatus_t status[MAX_TASKS];

    /* shift all traces left by 1 */
    for (int t = 0; t < tl_count; t++)
        memmove(tl_tracks[t].trace, tl_tracks[t].trace + 1, TIMELINE_W - 1);

    /* snapshot current task states */
    UBaseType_t n = uxTaskGetNumberOfTasks();
    if (n > MAX_TASKS) n = MAX_TASKS;
    n = uxTaskGetSystemState(status, n, NULL);

    for (UBaseType_t i = 0; i < n; i++) {
        int idx = tl_find_or_add(status[i].xHandle, status[i].pcTaskName);
        if (idx < 0) continue;

        char c;
        switch (status[i].eCurrentState) {
        case eRunning: c = '#'; break;
        case eReady:   c = '-'; break;
        case eBlocked: c = '.'; break;
        default:       c = ' '; break;
        }
        tl_tracks[idx].trace[TIMELINE_W - 1] = c;
    }
}

/* ── render table + timeline ─────────────────────── */

static void print_dash(int n) { while (n--) putchar('-'); }

static void render(int ansi_up) {
    printf("\r");

    if (ansi_up) {
        printf("\033[%dA", tl_count * 2 + 6);
    }

    /* ── timeline ── */
    printf(" Timeline (time >)                              last %ums\n",
           TIMELINE_W * SAMPLE_MS);
    print_dash(TIMELINE_W + 16); printf("\n");
    for (int t = 0; t < tl_count; t++) {
        char sym = tl_tracks[t].trace[TIMELINE_W - 1];
        printf("  %-12s ", tl_tracks[t].name);
        for (int x = 0; x < TIMELINE_W; x++) putchar(tl_tracks[t].trace[x]);
        printf("  %c\n", sym);
    }
    printf("\n");

    /* ── status table ── */
    {
        static TaskStatus_t s[MAX_TASKS];
        UBaseType_t n = uxTaskGetNumberOfTasks();
        if (n > MAX_TASKS) n = MAX_TASKS;
        n = uxTaskGetSystemState(s, n, NULL);

        printf(" %-12s %-5s %-4s %-6s %-6s %s\n",
               "Task", "State", "Prio", "Affin", "StkRm", "Handle");
        print_dash(12 + 1 + 5 + 1 + 4 + 1 + 6 + 1 + 6 + 1 + 10);
        printf("\n");

        for (UBaseType_t i = 0; i < n; i++) {
            const char *st = "???";
            switch (s[i].eCurrentState) {
            case eRunning:   st = "RUN";  break;
            case eReady:     st = "RDY";  break;
            case eBlocked:   st = "BLK";  break;
            case eSuspended: st = "SUS";  break;
            case eDeleted:   st = "DEL";  break;
            }

            char aff[8] = "—";
#if (configUSE_CORE_AFFINITY == 1) && (configNUMBER_OF_CORES > 1)
            if (s[i].uxCoreAffinityMask == 0xFF)
                snprintf(aff, sizeof(aff), "any");
            else if (s[i].uxCoreAffinityMask == (1 << 0))
                snprintf(aff, sizeof(aff), "core0");
            else if (s[i].uxCoreAffinityMask == (1 << 1))
                snprintf(aff, sizeof(aff), "core1");
            else
                snprintf(aff, sizeof(aff), "both");
#endif
            unsigned stk = s[i].usStackHighWaterMark;
            if (stk > 0xFFFF) stk = 0;

            printf(" %-12s %-5s %-4u %-6s %-6u 0x%08lX\n",
                   s[i].pcTaskName, st,
                   (unsigned)s[i].uxCurrentPriority,
                   aff, stk,
                   (unsigned long)(uintptr_t)s[i].xHandle);
        }
    }

    /* tick counter */
    printf(" tick %-6lu  total %-2u  @ %u ms/sample\n",
           (unsigned long)xTaskGetTickCount(),
           (unsigned)uxTaskGetNumberOfTasks(),
           SAMPLE_MS);
}

/* ── inspector task ──────────────────────────────── */

static void vInspectorTask(__unused void *param) {
    TickType_t xLast = xTaskGetTickCount();

    int first = 1;

    for (;;) {
        shift_and_sample();
        render(!first);
        first = 0;

        vTaskDelayUntil(&xLast, pdMS_TO_TICKS(SAMPLE_MS));
    }
}

void vStartInspector(UBaseType_t prio) {
    TaskHandle_t h;
    xTaskCreateAffinitySet(vInspectorTask, "inspector",
                           configMINIMAL_STACK_SIZE * 2,
                           NULL, prio,
                           (1 << 0) | (1 << 1), &h);
    (void)h;
}
