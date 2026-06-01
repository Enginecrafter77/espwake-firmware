#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <esp_wifi.h>
#include <esp_vfs_dev.h>
#include <stdlib.h>
#include <string.h>
#include <driver/gpio.h>
#include <driver/uart.h>

#include "espwake.h"
#include "espwake_sck.h"
#include "console_if.h"

#define TIME_MS(N) (N / portTICK_RATE_MS)

void user_wifi_init() {
    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_ESPWAKE_WIFI_SSID,
            .password = CONFIG_ESPWAKE_WIFI_PASSWORD,
            .threshold = {
                .authmode = WIFI_AUTH_WPA2_PSK
            }
        }
    };

    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, CONFIG_ESPWAKE_HOSTNAME));
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void console_init() {
    setvbuf(stdin, NULL, _IONBF, 0);
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    uart_config_t uart_config = {
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1
    };
    ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
}

static espwake_if_t espwake_if;
static espwake_t espwake;
static espwake_sck_service_t espwake_sck_service;

#ifdef CONFIG_ESPWAKE_DEBUG_SIMULATED_POWER_EVENTS
void irq_simulate_timer(void *arg) {
    switch(espwake_get_state(&espwake)) {
        case ESPWAKE_STATE_POWERING_ON:
            espwake_if.power_callback(true, espwake_if.power_callback_arg);
            break;
        case ESPWAKE_STATE_POWERING_OFF:
            espwake_if.power_callback(false, espwake_if.power_callback_arg);
            break;
        default:
            break;
    }
}
#endif

void app_main() {
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    
    espwake_if_init(&espwake_if);
    espwake_init(&espwake, &espwake_if);

    console_init();
    user_wifi_init();

    espwake_sck_service_create(&espwake_sck_service, &espwake);
    espwake_sck_service_set_password(&espwake_sck_service, CONFIG_ESPWAKE_CONTROL_PASSWORD);

    espwake_console_if_init(&espwake);

    #ifdef CONFIG_ESPWAKE_DEBUG_SIMULATED_POWER_EVENTS
    xTimerHandle sim_timer = xTimerCreate("power_sim_timer", TIME_MS(10000), pdTRUE, NULL, irq_simulate_timer);
    xTimerStart(sim_timer, 0);
    #endif
}
