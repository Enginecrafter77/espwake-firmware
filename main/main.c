#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>
#include <stdlib.h>
#include <string.h>
#include <driver/gpio.h>

#include "espwake.h"

#define TIME_MS(N) (N / portTICK_RATE_MS)

static espwake_if_t espwake_if;
static espwake_t espwake;

void check_power_status() {
    static bool prev_status = false;
    bool status = espwake_is_powered_on(&espwake);
    if (prev_status != status) {
        printf("Power level changed %d -> %d\n", prev_status, status);
        prev_status = status;
    }
}

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

void socket_service_task(void *arg) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);

    bind(sock, (const struct sockaddr*)&addr, sizeof(struct sockaddr_in));

    struct sockaddr_in peer;
    socklen_t peer_len;
    uint8_t buffer[4];
    while(1) {
        recvfrom(sock, buffer, 4, 0, (struct sockaddr*)&peer, &peer_len);
        printf("R CMD %d\n", buffer[0]);
        switch(buffer[0]) {
            case 0:
                espwake_power_on(&espwake);
                break;
            case 1:
                espwake_shutdown(&espwake);
                break;
            case 2:
                espwake_force_off(&espwake);
                break;
            default:
                break;
        }
    }
}

void app_main() {
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    
    espwake_if_init(&espwake_if);
    espwake_init(&espwake, &espwake_if);

    user_wifi_init();

    xTaskCreate(socket_service_task, "socket_service", 1024, NULL, 1, NULL);
    xTimerHandle power_timer = xTimerCreate("power_status_check", TIME_MS(100), pdTRUE, NULL, check_power_status);
    xTimerStart(power_timer, 0);
}
