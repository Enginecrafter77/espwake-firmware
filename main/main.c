#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>
#include <stdlib.h>
#include <string.h>

#include "espwake_if.h"

#define TIME_MS(N) (N / portTICK_RATE_MS)

typedef enum {
    SWITCH_SEQUENCE_NONE,
    SWITCH_SEQUENCE_START,
    SWITCH_SEQUENCE_SHUTDOWN,
    SWITCH_SEQUENCE_FORCEOFF
} switch_sequence_type_t;

switch_sequence_type_t seq_type;
xSemaphoreHandle semaphore;

#define HOLD_SWITCH(T) do { espwake_if_set_switch(true); vTaskDelay(TIME_MS(T)); espwake_if_set_switch(false); } while(0)

void switch_service_task(void *arg) {
    espwake_if_set_switch(false);
    while(1) {
        xSemaphoreTake(semaphore, portMAX_DELAY);
        switch(seq_type) {
            case SWITCH_SEQUENCE_START:
                HOLD_SWITCH(CONFIG_ESPWAKE_SEQ_START_HOLD_MS);
                break;
            case SWITCH_SEQUENCE_SHUTDOWN:
                HOLD_SWITCH(CONFIG_ESPWAKE_SEQ_SHUTDOWN_HOLD_MS);
                break;
            case SWITCH_SEQUENCE_FORCEOFF:
                HOLD_SWITCH(CONFIG_ESPWAKE_SEQ_FORCEOFF_HOLD_MS);
                break;
            default:
                break;
        }
        seq_type = SWITCH_SEQUENCE_NONE;
    }
}

void check_power_status() {
    static bool prev_status = false;
    bool status = espwake_if_poll_power_status();
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

void trigger_switch_sequence(switch_sequence_type_t type) {
    seq_type = type;
    xSemaphoreGive(semaphore);
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
                trigger_switch_sequence(SWITCH_SEQUENCE_START);
                break;
            case 1:
                trigger_switch_sequence(SWITCH_SEQUENCE_SHUTDOWN);
                break;
            case 2:
                trigger_switch_sequence(SWITCH_SEQUENCE_FORCEOFF);
                break;
            default:
                break;
        }
    }
}

void app_main() {
    espwake_if_init();

    user_wifi_init();

    semaphore = xSemaphoreCreateCounting(1, 0);

    xTaskCreate(switch_service_task, "switch_controller", 256, NULL, 2, NULL);
    xTaskCreate(socket_service_task, "socket_service", 1024, NULL, 1, NULL);

    xTimerHandle power_timer = xTimerCreate("power_status_check", TIME_MS(100), pdTRUE, NULL, check_power_status);
    xTimerStart(power_timer, 0);
}
