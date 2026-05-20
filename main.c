#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#include "inspector.h"

/* ── Experiment 2: Preemptive Scheduling ────────────
 *
 * Core 0: three tasks at different priorities
 *   HighTask (prio 3): sleeps 2600ms, runs 400ms (busy-wait)
 *    MidTask (prio 2): runs 800ms, sleeps 200ms
 *    LowTask (prio 1): always ready, runs only in the 200ms gap
 *
 *   Timeline:
 *   High  ----####--------------####--------------####---------
 *   Mid   ########----########----########----########----####
 *   Low   ....####....####....####....####....####....####....
 *           ^ 200ms gap where LowTask runs
 *
 * Core 1: Worker (prio 1) — independent, shows SMP parallelism
 */

#define PRIO_LOW    1
#define PRIO_MID    2
#define PRIO_HIGH   3
#define PRIO_INSP   3

/* ── Core 0: priority ladder ─────────────────────── */

static void vHighTask(__unused void *param) {
    TickType_t xLastWake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(3000));
        TickType_t start = xTaskGetTickCount();
        while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(400));
    }
}

static void vMidTask(__unused void *param) {
    TickType_t xLastWake = xTaskGetTickCount();
    for (;;) {
        TickType_t start = xTaskGetTickCount();
        while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(800));
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(1000));
    }
}

static void vLowTask(__unused void *param) {
    for (;;) {
        volatile uint32_t x = 0;
        for (int i = 0; i < 200000; i++) x++;
    }
}

/* ── Core 1: independent ─────────────────────────── */

static void vWorkerTask(__unused void *param) {
    for (;;) {
        volatile uint32_t x = 0;
        for (int i = 0; i < 200000; i++) x++;
    }
}

/* ── Heartbeat LED ───────────────────────────────── */

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

    printf("\n=== Exp 2: Preemptive Scheduling ===\n");

    TaskHandle_t h;

    xTaskCreateAffinitySet(vHighTask, "HighTask",
                           configMINIMAL_STACK_SIZE, NULL,
                           PRIO_HIGH, (1 << 0), &h);

    xTaskCreateAffinitySet(vMidTask, "MidTask",
                           configMINIMAL_STACK_SIZE, NULL,
                           PRIO_MID, (1 << 0), &h);

    xTaskCreateAffinitySet(vLowTask, "LowTask",
                           configMINIMAL_STACK_SIZE, NULL,
                           PRIO_LOW, (1 << 0), &h);

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
