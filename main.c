#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "inspector.h"

#define PRIO_LOW     1
#define PRIO_MID     2
#define PRIO_HIGH    3
#define PRIO_MONITOR 3

static SemaphoreHandle_t xSem = NULL;

static void vLedTask(__unused void *params) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    for (;;) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(800));
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

static void vAlwaysReadyLow(__unused void *params) {
    volatile uint32_t x = 0;
    for (;;) {
        x += 1;
        taskYIELD();
    }
}

static void vAlwaysReadyMid(__unused void *params) {
    volatile uint32_t x = 0;
    for (;;) {
        x += 1;
        taskYIELD();
    }
}

static void vBlockedTask(__unused void *params) {
    for (;;) {
        xSemaphoreTake(xSem, pdMS_TO_TICKS(3000));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void vSignallerTask(__unused void *params) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        xSemaphoreGive(xSem);
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
    printf("\n--- FreeRTOS TCB Inspector ---\n");
    printf("RP2350 dual Cortex-M33  |  %u cores  %u priorities\n",
           configNUMBER_OF_CORES, configMAX_PRIORITIES);

    xSem = xSemaphoreCreateBinary();
    configASSERT(xSem);

    TaskHandle_t h;

    xTaskCreateAffinitySet(vLedTask, "led", configMINIMAL_STACK_SIZE,
                           NULL, PRIO_LOW, (1 << 0), &h);

    xTaskCreateAffinitySet(vAlwaysReadyLow, "busyL",
                           configMINIMAL_STACK_SIZE,
                           NULL, PRIO_LOW, (1 << 0), &h);

    xTaskCreateAffinitySet(vAlwaysReadyMid, "busyM",
                           configMINIMAL_STACK_SIZE,
                           NULL, PRIO_MID, (1 << 1), &h);

    xTaskCreateAffinitySet(vBlockedTask, "blocked",
                           configMINIMAL_STACK_SIZE,
                           NULL, PRIO_HIGH, (1 << 0) | (1 << 1), &h);

    xTaskCreateAffinitySet(vSignallerTask, "signal",
                           configMINIMAL_STACK_SIZE,
                           NULL, PRIO_HIGH, (1 << 0) | (1 << 1), &h);

    vStartInspector(PRIO_MONITOR);

    vTaskStartScheduler();

    for (;;);
}
