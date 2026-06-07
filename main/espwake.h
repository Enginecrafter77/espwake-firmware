#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>

#include "espwake_if.h"

typedef enum {
    ESPWAKE_DSTATE_RUNNING,
    ESPWAKE_DSTATE_SUSPENDED,
    ESPWAKE_DSTATE_POWER_OFF
} espwake_detected_state_t;

typedef void (*espwake_state_detector_callback_t)(espwake_detected_state_t new_state, void *arg);

typedef struct {
    espwake_if_t *interface;
    xTimerHandle eval_timer;
    uint32_t prev_edge_time;
    uint32_t last_edge_time;
    bool last_edge_signal;
    espwake_detected_state_t state;
    espwake_state_detector_callback_t callback;
    void *callback_arg;
} espwake_state_detector_t;

void espwake_state_detector_init(espwake_state_detector_t *detector, espwake_if_t *interface);
void espwake_state_detector_destroy(espwake_state_detector_t *detector);
static inline void espwake_state_detector_set_callback(espwake_state_detector_t *detector, espwake_state_detector_callback_t callback, void *arg) { detector->callback = callback; detector->callback_arg = arg; }
const char* espwake_detected_state_name(espwake_detected_state_t state);

typedef enum {
    ESPWAKE_SWITCH_SEQUENCE_NONE,
    ESPWAKE_SWITCH_SEQUENCE_START,
    ESPWAKE_SWITCH_SEQUENCE_SHUTDOWN,
    ESPWAKE_SWITCH_SEQUENCE_FORCEOFF
} espwake_switch_sequence_t;

typedef enum {
    ESPWAKE_STATE_POWERING_ON,
    ESPWAKE_STATE_POWERED_ON,
    ESPWAKE_STATE_SUSPENDED,
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
    espwake_state_detector_t detector;
} espwake_t;

void espwake_init(espwake_t *espwake, espwake_if_t *interface);
void espwake_power_on(espwake_t *espwake);
void espwake_shutdown(espwake_t *espwake);
void espwake_force_off(espwake_t *espwake);
void espwake_destroy(espwake_t *espwake);

static inline espwake_state_t espwake_get_state(espwake_t *espwake) { return espwake->state; }
const char* espwake_state_name(espwake_state_t state);
