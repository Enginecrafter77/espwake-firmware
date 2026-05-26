#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "espwake_if.h"

typedef enum {
    ESPWAKE_SWITCH_SEQUENCE_NONE,
    ESPWAKE_SWITCH_SEQUENCE_START,
    ESPWAKE_SWITCH_SEQUENCE_SHUTDOWN,
    ESPWAKE_SWITCH_SEQUENCE_FORCEOFF
} espwake_switch_sequence_t;

typedef struct {
    xSemaphoreHandle seq_ready_semaphore;
    xSemaphoreHandle seq_empty_semaphore;
    xTaskHandle switch_controller_task;
    espwake_switch_sequence_t sequence;
    espwake_if_t *interface;
} espwake_t;

void espwake_init(espwake_t *espwake, espwake_if_t *interface);
bool espwake_is_powered_on(espwake_t *espwake);
void espwake_power_on(espwake_t *espwake);
void espwake_shutdown(espwake_t *espwake);
void espwake_force_off(espwake_t *espwake);
void espwake_destroy(espwake_t *espwake);
