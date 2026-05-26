#include "espwake.h"
#include "espwake_if.h"

#define TIME_MS(N) (N / portTICK_RATE_MS)

static inline void espwake_hold_switch(espwake_t *espwake, uint16_t ms) {
    espwake_if_set_switch(espwake->interface, true);
    vTaskDelay(TIME_MS(ms));
    espwake_if_set_switch(espwake->interface, false); 
}

static void espwake_switch_control_task(void *arg) {
    espwake_t *espwake = (espwake_t*)arg;

    espwake_if_set_switch(espwake->interface, false);
    while(1) {
        xSemaphoreTake(espwake->seq_ready_semaphore, portMAX_DELAY);
        switch(espwake->sequence) {
            case ESPWAKE_SWITCH_SEQUENCE_START:
                espwake_hold_switch(espwake, CONFIG_ESPWAKE_SEQ_START_HOLD_MS);
                break;
            case ESPWAKE_SWITCH_SEQUENCE_SHUTDOWN:
                espwake_hold_switch(espwake, CONFIG_ESPWAKE_SEQ_SHUTDOWN_HOLD_MS);
                break;
            case ESPWAKE_SWITCH_SEQUENCE_FORCEOFF:
                espwake_hold_switch(espwake, CONFIG_ESPWAKE_SEQ_FORCEOFF_HOLD_MS);
                break;
            default:
                break;
        }
        espwake->sequence = ESPWAKE_SWITCH_SEQUENCE_NONE;
        xSemaphoreGive(espwake->seq_empty_semaphore);
    }
}

static void espwake_dispatch_sequence(espwake_t *espwake, espwake_switch_sequence_t sequence) {
    xSemaphoreTake(espwake->seq_empty_semaphore, portMAX_DELAY);
    espwake->sequence = sequence;
    xSemaphoreGive(espwake->seq_ready_semaphore);
}

void espwake_init(espwake_t *espwake, espwake_if_t *interface) {
    espwake->interface = interface;
    espwake->seq_ready_semaphore = xSemaphoreCreateCounting(1, 0);
    espwake->seq_empty_semaphore = xSemaphoreCreateCounting(1, 1);
    espwake->sequence = ESPWAKE_SWITCH_SEQUENCE_NONE;
    xTaskCreate(espwake_switch_control_task, "espwake_switch_controller", 256, espwake, 2, &espwake->switch_controller_task);
}

bool espwake_is_powered_on(espwake_t *espwake) {
    return espwake_if_poll_power_status(espwake->interface);
}

void espwake_destroy(espwake_t *espwake) {
    vTaskDelete(espwake->switch_controller_task);
    vSemaphoreDelete(espwake->seq_ready_semaphore);
}

void espwake_power_on(espwake_t *espwake) {
    if (espwake_if_poll_power_status(espwake->interface)) {
        return;
    }
    espwake_dispatch_sequence(espwake, ESPWAKE_SWITCH_SEQUENCE_START);
}

void espwake_shutdown(espwake_t *espwake) {
    if (!espwake_if_poll_power_status(espwake->interface)) {
        return;
    }
    espwake_dispatch_sequence(espwake, ESPWAKE_SWITCH_SEQUENCE_SHUTDOWN);
}

void espwake_force_off(espwake_t *espwake) {
    if (!espwake_if_poll_power_status(espwake->interface)) {
        return;
    }
    espwake_dispatch_sequence(espwake, ESPWAKE_SWITCH_SEQUENCE_FORCEOFF);
}
