#include "config.h"
#include <sdkconfig.h>
#include <stdio.h>
#include <string.h>
#include <esp_system.h>

#include "util.h"

#define CONFIG_PATH "/config/espwake"
#define static_assert _Static_assert

static void make_hostname(char *out) {
    #ifdef CONFIG_ESPWAKE_HOSTNAME_APPEND_MAC
    static_assert((sizeof(CONFIG_ESPWAKE_DEFAULT_HOSTNAME)+8) < ESPWAKE_CONFIG_HOSTNAME_LENGTH, "Hostname must be at most 23 characters long.");
    strncpy(out, CONFIG_ESPWAKE_DEFAULT_HOSTNAME, ESPWAKE_CONFIG_HOSTNAME_LENGTH);
    char *base = out + sizeof(CONFIG_ESPWAKE_DEFAULT_HOSTNAME);
    base[-1] = '-';
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    hex2str(base, mac + 3, 6);
    #else
    static_assert((sizeof(CONFIG_ESPWAKE_DEFAULT_HOSTNAME)+1) < ESPWAKE_CONFIG_HOSTNAME_LENGTH, "Hostname must be at most 31 characters long.");
    strncpy(out, CONFIG_ESPWAKE_DEFAULT_HOSTNAME, ESPWAKE_CONFIG_HOSTNAME_LENGTH);
    #endif
}

void espwake_config_defaults(espwake_config_t *config) {
    config->wifi_ssid[0] = 0;
    config->wifi_password[0] = 0;
    strncpy(config->control_password, CONFIG_ESPWAKE_DEFAULT_CONTROL_PASSWORD, ESPWAKE_CONFIG_CONTROL_PASSWORD_LENGTH);
    make_hostname(config->hostname);
}

void espwake_config_load(espwake_config_t *config) {
    FILE *file = fopen(CONFIG_PATH, "r");
    if (file != NULL) {
        fread(config->wifi_ssid, sizeof(char), ESPWAKE_CONFIG_WIFI_SSID_LENGTH, file);
        fread(config->wifi_password, sizeof(char), ESPWAKE_CONFIG_WIFI_PASSWORD_LENGTH, file);
        fread(config->control_password, sizeof(char), ESPWAKE_CONFIG_CONTROL_PASSWORD_LENGTH, file);
        fread(config->hostname, sizeof(char), ESPWAKE_CONFIG_HOSTNAME_LENGTH, file);
        fclose(file);
    } else {
        espwake_config_defaults(config);
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
