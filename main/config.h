#pragma once
#include <esp_wifi_types.h>

#define ESPWAKE_CONFIG_WIFI_SSID_LENGTH 32
#define ESPWAKE_CONFIG_WIFI_PASSWORD_LENGTH 64
#define ESPWAKE_CONFIG_CONTROL_PASSWORD_LENGTH 16

typedef struct {
    char wifi_ssid[ESPWAKE_CONFIG_WIFI_SSID_LENGTH];
    char wifi_password[ESPWAKE_CONFIG_WIFI_PASSWORD_LENGTH];
    char control_password[ESPWAKE_CONFIG_CONTROL_PASSWORD_LENGTH];
} espwake_config_t;

extern const espwake_config_t espwake_config_defaults;

void espwake_config_load(espwake_config_t *config);
void espwake_config_save(const espwake_config_t *config);

void espwake_config_wifi(wifi_config_t *wifi, const espwake_config_t *config);