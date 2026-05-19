#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define LED_PIN     PICO_DEFAULT_LED_PIN
#define GPIO2_PIN   2

#define BLINKY_PRIORITY     ( tskIDLE_PRIORITY + 1 )
#define MONITOR_PRIORITY    ( tskIDLE_PRIORITY + 2 )

#define QUEUE_LENGTH     4
#define QUEUE_ITEM_SIZE  sizeof(uint32_t)

static QueueHandle_t xQueue = NULL;

static void init_hardware(void) {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(GPIO2_PIN);
    gpio_set_dir(GPIO2_PIN, GPIO_OUT);
}

static void boot_indicate(void) {
    for (int i = 0; i < 3; i++) {
        gpio_put(LED_PIN, 1);
        busy_wait_ms(100);
        gpio_put(LED_PIN, 0);
        busy_wait_ms(100);
    }
}

static void vLedTask(__unused void *params) {
    TickType_t xLastWake = xTaskGetTickCount();

    for (;;) {
        gpio_put(LED_PIN, 1);
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(500));
        gpio_put(LED_PIN, 0);
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(500));

        static uint32_t ulCount = 0;
        if (xQueue) {
            xQueueSend(xQueue, &ulCount, 0);
        }
        ulCount++;
    }
}

static void vGpioTask(__unused void *params) {
    TickType_t xLastWake = xTaskGetTickCount();

    for (;;) {
        gpio_put(GPIO2_PIN, 1);
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(100));
        gpio_put(GPIO2_PIN, 0);
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(100));

        static uint32_t ulCount = 0;
        if (xQueue) {
            xQueueSend(xQueue, &ulCount, 0);
        }
        ulCount++;
    }
}

static void vMonitorTask(__unused void *params) {
    uint32_t ulValue;

    for (;;) {
        if (xQueueReceive(xQueue, &ulValue, portMAX_DELAY) == pdPASS) {
            printf("[core %d] tick %lu\n", portGET_CORE_ID(), ulValue);
        }
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("stack overflow: %s\n", pcTaskName);
    __asm volatile("bkpt #0");
    for (;;);
}

int main(void) {
    init_hardware();
    boot_indicate();

    xQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    configASSERT(xQueue);

    printf("FreeRTOS SMP starting on RP2350...\n");

    TaskHandle_t xLedTask = NULL, xGpioTask = NULL, xMonTask = NULL;

    xTaskCreateAffinitySet(vLedTask, "led", configMINIMAL_STACK_SIZE,
                           NULL, BLINKY_PRIORITY, (1 << 0), &xLedTask);
    configASSERT(xLedTask);

    xTaskCreateAffinitySet(vGpioTask, "gpio", configMINIMAL_STACK_SIZE,
                           NULL, BLINKY_PRIORITY, (1 << 1), &xGpioTask);
    configASSERT(xGpioTask);

    xTaskCreateAffinitySet(vMonitorTask, "mon", configMINIMAL_STACK_SIZE,
                           NULL, MONITOR_PRIORITY,
                           (1 << 0) | (1 << 1), &xMonTask);
    configASSERT(xMonTask);

    vTaskStartScheduler();

    for (;;);
}
