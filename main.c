#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "inspector.h"

/* ── Experiment 3: Priority Inversion ──────────────
 *
 * Toggle USE_MUTEX to compare two locking primitives:
 *
 *   USE_MUTEX=0  →  binary semaphore (no ownership, no inheritance)
 *   USE_MUTEX=1  →  mutex (ownership + priority inheritance)
 *
 * Core 0: three tasks, one lock
 *   LowTask  (prio 1): takes lock, holds 5s, releases
 *   MidTask  (prio 2): CPU-bound, no lock at all
 *   HighTask (prio 3): waits 1s, tries lock → runs → releases
 *
 * Without inheritance (USE_MUTEX=0):
 *   LowTask holds lock → HighTask blocked on lock → MidTask runs
 *   → LowTask starved → HighTask stuck = priority inversion
 *
 * With inheritance (USE_MUTEX=1):
 *   LowTask holds lock → HighTask blocked → LowTask inherits prio 3
 *   → LowTask runs (MidTask can't preempt) → releases → HighTask runs
 */

#define USE_MUTEX   1   /* toggle: 0 = inversion, 1 = inheritance */

#define PRIO_LOW    1
#define PRIO_MID    2
#define PRIO_HIGH   3
#define PRIO_INSP   3

static SemaphoreHandle_t xLock = NULL;

/* ── Core 0: priority inversion drama ───────────── */

static void vLowTask(__unused void *param) {
    TickType_t xLastWake = xTaskGetTickCount();
    for (;;) {
        xSemaphoreTake(xLock, portMAX_DELAY);

        TickType_t start = xTaskGetTickCount();
        while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(5000));

        xSemaphoreGive(xLock);
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(6000));
    }
}

static void vMidTask(__unused void *param) {
    for (;;) {
        volatile uint32_t x = 0;
        for (int i = 0; i < 500000; i++) x++;
    }
}

static void vHighTask(__unused void *param) {
    TickType_t xLastWake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(6000));

        vTaskDelay(pdMS_TO_TICKS(1000));

        xSemaphoreTake(xLock, portMAX_DELAY);

        TickType_t start = xTaskGetTickCount();
        while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(200));

        xSemaphoreGive(xLock);
    }
}

/* ── Worker on Core 1 ───────────────────────────── */

static void vWorkerTask(__unused void *param) {
    for (;;) {
        volatile uint32_t x = 0;
        for (int i = 0; i < 200000; i++) x++;
    }
}

/* ── LED heartbeat ──────────────────────────────── */

static void vLedTask(__unused void *param) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    TickType_t xLastWake = xTaskGetTickCount();
    for (;;) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(900));
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(100));
    }
}

/* ── ─────────────────────────────────────────────── */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("stack overflow: %s\n", pcTaskName);
    __asm volatile("bkpt #0");
    for (;;);
}

int main(void) {
    stdio_init_all();
    sleep_ms(1500);

#if USE_MUTEX
    xLock = xSemaphoreCreateMutex();
    printf("\n=== Exp 3: Priority Inheritance (MUTEX) ===\n");
    printf("LowTask inherits prio 3 when HighTask waits\n");
#else
    xLock = xSemaphoreCreateBinary();
    xSemaphoreGive(xLock);
    printf("\n=== Exp 3: Priority Inversion (SEMAPHORE) ===\n");
    printf("Watch MidTask preempt LowTask while HighTask waits\n");
#endif
    configASSERT(xLock);

    TaskHandle_t h;

    xTaskCreateAffinitySet(vLowTask, "LowTask",
                           configMINIMAL_STACK_SIZE, NULL,
                           PRIO_LOW, (1 << 0), &h);

    xTaskCreateAffinitySet(vMidTask, "MidTask",
                           configMINIMAL_STACK_SIZE, NULL,
                           PRIO_MID, (1 << 0), &h);

    xTaskCreateAffinitySet(vHighTask, "HighTask",
                           configMINIMAL_STACK_SIZE, NULL,
                           PRIO_HIGH, (1 << 0), &h);

    xTaskCreateAffinitySet(vWorkerTask, "Worker",
                           configMINIMAL_STACK_SIZE, NULL,
                           PRIO_LOW, (1 << 1), &h);

    xTaskCreateAffinitySet(vLedTask, "led",
                           configMINIMAL_STACK_SIZE, NULL,
                           PRIO_LOW, (1 << 0) | (1 << 1), &h);

    vStartInspector(PRIO_INSP);

    vTaskStartScheduler();
    for (;;);
}
