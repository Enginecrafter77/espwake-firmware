#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>
#include <stdlib.h>
#include <string.h>
#include <driver/gpio.h>

#include "espwake.h"

#define TIME_MS(N) (N / portTICK_RATE_MS)

#define ESPWAKE_PROTO_COMMAND_CHECK 0
#define ESPWAKE_PROTO_COMMAND_START 1
#define ESPWAKE_PROTO_COMMAND_SHUTDOWN 2
#define ESPWAKE_PROTO_COMMAND_FORCEOFF 3

#define ESPWAKE_PROTO_RESP_POWERED_OFF 0
#define ESPWAKE_PROTO_RESP_POWERING_OFF 1
#define ESPWAKE_PROTO_RESP_POWERING_ON 2
#define ESPWAKE_PROTO_RESP_POWERED_ON 3

static espwake_if_t espwake_if;
static espwake_t espwake;

static uint8_t espwake_proto_map_state(espwake_state_t state) {
    switch (state) {
        case ESPWAKE_STATE_POWERED_OFF:
            return ESPWAKE_PROTO_RESP_POWERED_OFF;
        case ESPWAKE_STATE_POWERING_OFF:
            return ESPWAKE_PROTO_RESP_POWERING_OFF;
        case ESPWAKE_STATE_POWERING_ON:
            return ESPWAKE_PROTO_RESP_POWERING_ON;
        case ESPWAKE_STATE_POWERED_ON:
            return ESPWAKE_PROTO_RESP_POWERED_ON;
    }
    return ESPWAKE_PROTO_RESP_POWERED_OFF;
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
    socklen_t peer_len = sizeof(struct sockaddr_in);
    uint8_t buffer[4];
    while(1) {
        recvfrom(sock, buffer, 4, 0, (struct sockaddr*)&peer, &peer_len);
        switch(buffer[0]) {
            case ESPWAKE_PROTO_COMMAND_START:
                espwake_power_on(&espwake);
                break;
            case ESPWAKE_PROTO_COMMAND_SHUTDOWN:
                espwake_shutdown(&espwake);
                break;
            case ESPWAKE_PROTO_COMMAND_FORCEOFF:
                espwake_force_off(&espwake);
                break;
            default:
                buffer[0] = espwake_proto_map_state(espwake_get_state(&espwake));
                sendto(sock, buffer, 4, 0, (const struct sockaddr*)&peer, peer_len);
                break;
        }
    }
}

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

    user_wifi_init();

    xTaskCreate(socket_service_task, "socket_service", 2048, NULL, 1, NULL);

    #ifdef CONFIG_ESPWAKE_DEBUG_SIMULATED_POWER_EVENTS
    xTimerHandle sim_timer = xTimerCreate("power_sim_timer", TIME_MS(10000), pdTRUE, NULL, irq_simulate_timer);
    xTimerStart(sim_timer, 0);
    #endif
}
