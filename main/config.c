#include "config.h"
#include <sdkconfig.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_PATH "/config/espwake"

const espwake_config_t espwake_config_defaults = {
    .wifi_ssid = CONFIG_ESPWAKE_WIFI_SSID,
    .wifi_password = CONFIG_ESPWAKE_WIFI_PASSWORD,
    .control_password = CONFIG_ESPWAKE_CONTROL_PASSWORD,
    .hostname = CONFIG_ESPWAKE_HOSTNAME
};

void espwake_config_load(espwake_config_t *config) {
    FILE *file = fopen(CONFIG_PATH, "r");
    if (file != NULL) {
        fread(config->wifi_ssid, sizeof(char), ESPWAKE_CONFIG_WIFI_SSID_LENGTH, file);
        fread(config->wifi_password, sizeof(char), ESPWAKE_CONFIG_WIFI_PASSWORD_LENGTH, file);
        fread(config->control_password, sizeof(char), ESPWAKE_CONFIG_CONTROL_PASSWORD_LENGTH, file);
        fread(config->hostname, sizeof(char), ESPWAKE_CONFIG_HOSTNAME_LENGTH, file);
        fclose(file);
    } else {
        *config = espwake_config_defaults;
    }
}

void espwake_config_save(const espwake_config_t *config) {
    FILE *file = fopen(CONFIG_PATH, "w");
    if(file == NULL) {
        return;
    }
    fwrite(config->wifi_ssid, sizeof(char), ESPWAKE_CONFIG_WIFI_SSID_LENGTH, file);
    fwrite(config->wifi_password, sizeof(char), ESPWAKE_CONFIG_WIFI_PASSWORD_LENGTH, file);
    fwrite(config->control_password, sizeof(char), ESPWAKE_CONFIG_CONTROL_PASSWORD_LENGTH, file);
    fwrite(config->hostname, sizeof(char), ESPWAKE_CONFIG_HOSTNAME_LENGTH, file);
    fflush(file);
    fclose(file);
}

void espwake_config_wifi(wifi_config_t *wifi, const espwake_config_t *config) {
    memset(wifi, 0, sizeof(wifi_config_t));
    strncpy((char*)wifi->sta.ssid, config->wifi_ssid, ESPWAKE_CONFIG_WIFI_SSID_LENGTH);
    strncpy((char*)wifi->sta.password, config->wifi_password, ESPWAKE_CONFIG_WIFI_PASSWORD_LENGTH);
    wifi->sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
}
