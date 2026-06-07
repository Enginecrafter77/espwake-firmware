#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_console.h>
#include <esp_wifi.h>
#include <linenoise/linenoise.h>
#include <string.h>

#include "espwake_sck.h"
#include "console_if.h"
#include "config.h"

extern espwake_t espwake;
extern espwake_sck_service_t espwake_sck_service;

static int espwake_cmd_start(int argc, char **argv) {
    espwake_power_on(&espwake);
    return 0;
}

static int espwake_cmd_shutdown(int argc, char **argv) {
    espwake_shutdown(&espwake);
    return 0;
}

static int espwake_cmd_forceoff(int argc, char **argv) {
    espwake_force_off(&espwake);
    return 0;
}

static int espwake_cmd_status(int argc, char **argv) {
    puts(espwake_state_name(espwake_get_state(&espwake)));
    return 0;
}

static int espwake_cmd_set_password(int argc, char **argv) {
    if (argc < 2) {
        return 1;
    }

    espwake_config_t config;
    espwake_config_load(&config);
    strncpy(config.control_password, argv[1], ESPWAKE_CONFIG_CONTROL_PASSWORD_LENGTH);
    espwake_config_save(&config);

    espwake_sck_service_set_password(&espwake_sck_service, argv[1]);
    return 0;
}

static int espwake_cmd_set_wifi_auth(int argc, char **argv) {
    if (argc < 3) {
        return 1;
    }

    espwake_config_t config;
    espwake_config_load(&config);
    strncpy(config.wifi_ssid, argv[1], ESPWAKE_CONFIG_WIFI_SSID_LENGTH);
    strncpy(config.wifi_password, argv[2], ESPWAKE_CONFIG_WIFI_PASSWORD_LENGTH);
    espwake_config_save(&config);

    ESP_ERROR_CHECK(esp_wifi_disconnect());
    wifi_config_t wificfg;
    espwake_config_wifi(&wificfg, &config);
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wificfg));
    ESP_ERROR_CHECK(esp_wifi_connect());

    return 0;
}

static int espwake_cmd_set_hostname(int argc, char **argv) {
    espwake_config_t config;
    espwake_config_load(&config);

    if (argc < 2) {
        puts(config.hostname);
        return 0;
    }

    strncpy(config.hostname, argv[1], ESPWAKE_CONFIG_HOSTNAME_LENGTH);
    espwake_config_save(&config);

    ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, config.hostname));

    return 0;
}

static int espwake_cmd_test(int argc, char **argv) {
    espwake_config_t config;
    espwake_config_defaults(&config);
    printf("default hostname: %s\n", config.hostname);
    return 0;
}

void espwake_serial_operator_task(void *arg) {
    int probe_status = linenoiseProbe();
    if (probe_status) {
        linenoiseSetDumbMode(1);
    }

    while (true) {
        char* line = linenoise("espwake> ");
        if (line == NULL) {
            continue;
        }
        
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
        } else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        linenoiseFree(line);
    }
}

#define REGISTER_CMD(C, F, H, A) do { const esp_console_cmd_t cmd = {.command = C, .help = H, .func = F, .hint = A}; ESP_ERROR_CHECK(esp_console_cmd_register(&cmd)); } while(0)

void espwake_console_if_init() {
    esp_console_config_t console_config = {
        .max_cmdline_args = 8,
        .max_cmdline_length = 256
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));
    linenoiseSetMultiLine(1);

    REGISTER_CMD("start", espwake_cmd_start, "Starts the connected PC", NULL);
    REGISTER_CMD("shutdown", espwake_cmd_shutdown, "Sends an ACPI shutdown to the PC", NULL);
    REGISTER_CMD("forceoff", espwake_cmd_forceoff, "Forcefully powers down the connected PC", NULL);
    REGISTER_CMD("status", espwake_cmd_status, "Shows the current status of the connected PC", NULL);
    REGISTER_CMD("passwd", espwake_cmd_set_password, "Set authentication password", "<password>");
    REGISTER_CMD("hostname", espwake_cmd_set_hostname, "Set device hostname", "<hostname>");
    REGISTER_CMD("wifi", espwake_cmd_set_wifi_auth, "Set wifi SSID and password", "<ssid> <password>");
    REGISTER_CMD("test", espwake_cmd_test, "Test command", NULL);
    esp_console_register_help_command();

    xTaskCreate(espwake_serial_operator_task, "serial_operator_task", 2048, NULL, 1, NULL);
}
