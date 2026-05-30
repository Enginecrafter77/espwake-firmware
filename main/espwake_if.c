#include "espwake_if.h"

#include <driver/gpio.h>
#include <stdbool.h>

#define GPIO_PIN_SWCTRL GPIO_NUM_4
#define GPIO_PIN_PWROFF GPIO_NUM_5

static void __espwake_pwroff_irq_handler(void *arg);
static bool __espwake_pwroff_poller();
static void __espwake_switch_setter(bool value);

void espwake_if_init(espwake_if_t *interface) {
    gpio_config_t cfg;
    cfg.mode = GPIO_MODE_OUTPUT;
    cfg.pin_bit_mask = (1 << GPIO_PIN_SWCTRL);
    cfg.pull_down_en = 0;
    cfg.pull_up_en = 0;
    cfg.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&cfg));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_PIN_SWCTRL, 0));

    cfg.mode = GPIO_MODE_INPUT;
    cfg.pin_bit_mask = (1 << GPIO_PIN_PWROFF);
    cfg.pull_down_en = 0;
    cfg.pull_up_en = 1;
    cfg.intr_type = GPIO_INTR_ANYEDGE;
    ESP_ERROR_CHECK(gpio_config(&cfg));
    
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_PIN_PWROFF, __espwake_pwroff_irq_handler, interface));

    #ifdef CONFIG_ESPWAKE_DEBUG_NODEMCU_LED
    cfg.mode = GPIO_MODE_OUTPUT;
    cfg.pin_bit_mask = (1 << 16);
    cfg.pull_down_en = 0;
    cfg.pull_up_en = 0;
    cfg.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&cfg));
    #endif

    interface->switch_setter = __espwake_switch_setter;
    interface->power_poller = __espwake_pwroff_poller;
    interface->power_callback = NULL;
    interface->power_callback_arg = NULL;
}

static void __espwake_switch_setter(bool switch_value) {
    gpio_set_level(GPIO_PIN_SWCTRL, (switch_value ? 1 : 0));

    #ifdef CONFIG_ESPWAKE_DEBUG_NODEMCU_LED
    gpio_set_level(GPIO_NUM_16, (switch_value ? 0 : 1));
    #endif
}

static void __espwake_pwroff_irq_handler(void *arg) {
    espwake_if_t *interface = (espwake_if_t*)arg;
    if (interface->power_callback == NULL) {
        return;
    }
    (interface->power_callback)(espwake_if_poll_power_status(interface), interface->power_callback_arg);
}

static bool __espwake_pwroff_poller() {
    return !gpio_get_level(GPIO_PIN_PWROFF);
}
