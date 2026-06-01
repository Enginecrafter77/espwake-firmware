#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_console.h>
#include <linenoise/linenoise.h>
#include "console_if.h"

static espwake_t *__espwake_ptr;

static const char* espwake_state_to_string(espwake_state_t state) {
    switch(state) {
        case ESPWAKE_STATE_POWERED_OFF:
            return "power_off";
        case ESPWAKE_STATE_POWERING_OFF:
            return "powering_off";
        case ESPWAKE_STATE_POWERING_ON:
            return "powering_on";
        case ESPWAKE_STATE_POWERED_ON:
            return "powered_on";
    }
    return "<<unknown>>";
}

static int espwake_cmd_start(int argc, char **argv) {
    espwake_power_on(__espwake_ptr);
    return 0;
}

static int espwake_cmd_shutdown(int argc, char **argv) {
    espwake_shutdown(__espwake_ptr);
    return 0;
}

static int espwake_cmd_forceoff(int argc, char **argv) {
    espwake_force_off(__espwake_ptr);
    return 0;
}

static int espwake_cmd_status(int argc, char **argv) {
    puts(espwake_state_to_string(espwake_get_state(__espwake_ptr)));
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

void espwake_console_if_init(espwake_t *espwake) {
    __espwake_ptr = espwake;
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
    esp_console_register_help_command();

    xTaskCreate(espwake_serial_operator_task, "serial_operator_task", 1024, NULL, 1, NULL);
}
