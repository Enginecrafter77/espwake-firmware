#include "espwake_if.h"

#include <driver/gpio.h>
#include <stdbool.h>

#define GPIO_PIN_SWCTRL GPIO_NUM_4
#define GPIO_PIN_PWROFF GPIO_NUM_5

#define ESPWAKE_VISUALIZE_SWITCH_BY_LED

void espwake_if_init() {
    gpio_config_t cfg;
    cfg.mode = GPIO_MODE_OUTPUT;
    cfg.pin_bit_mask = (1 << GPIO_PIN_SWCTRL);
    cfg.pull_down_en = 0;
    cfg.pull_up_en = 0;
    cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&cfg);
    gpio_set_level(GPIO_PIN_SWCTRL, 0);

    cfg.mode = GPIO_MODE_INPUT;
    cfg.pin_bit_mask = (1 << GPIO_PIN_PWROFF);
    cfg.pull_down_en = 0;
    cfg.pull_up_en = 1;
    cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&cfg);

    #ifdef ESPWAKE_VISUALIZE_SWITCH_BY_LED
    cfg.mode = GPIO_MODE_OUTPUT;
    cfg.pin_bit_mask = (1 << 16);
    cfg.pull_down_en = 0;
    cfg.pull_up_en = 0;
    cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&cfg);
    #endif
}

void espwake_if_set_switch(bool switch_value) {
    gpio_set_level(GPIO_PIN_SWCTRL, (switch_value ? 1 : 0));

    #ifdef ESPWAKE_VISUALIZE_SWITCH_BY_LED
    gpio_set_level(GPIO_NUM_16, (switch_value ? 0 : 1));
    #endif
}

bool espwake_if_poll_power_status() {
    return !gpio_get_level(GPIO_PIN_PWROFF);
}
