#include "pico/stdlib.h"

// Pico 2 官方 LED 定义
#ifndef PICO_DEFAULT_LED_PIN
#error "This board does not have a default LED pin"
#endif

int main(void)
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while (true) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(500);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(500);
    }
}
