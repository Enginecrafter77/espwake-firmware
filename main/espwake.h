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

typedef enum {
    ESPWAKE_STATE_POWERING_ON,
    ESPWAKE_STATE_POWERED_ON,
    ESPWAKE_STATE_POWERING_OFF,
    ESPWAKE_STATE_POWERED_OFF
} espwake_state_t;

typedef struct {
    xSemaphoreHandle seq_ready_semaphore;
    xSemaphoreHandle seq_empty_semaphore;
    xTaskHandle switch_controller_task;
    espwake_switch_sequence_t sequence;
    espwake_state_t state;
    espwake_if_t *interface;
} espwake_t;

void espwake_init(espwake_t *espwake, espwake_if_t *interface);
void espwake_power_on(espwake_t *espwake);
void espwake_shutdown(espwake_t *espwake);
void espwake_force_off(espwake_t *espwake);
void espwake_destroy(espwake_t *espwake);

static inline espwake_state_t espwake_get_state(espwake_t *espwake) { return espwake->state; }
