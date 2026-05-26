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

static void pwroff_irq_handler(bool poweron, void *arg) {
    espwake_t *espwake = (espwake_t*)arg;
    if (poweron) {
        switch(espwake->state) {
            case ESPWAKE_STATE_POWERED_OFF:
            case ESPWAKE_STATE_POWERING_ON:
                espwake->state = ESPWAKE_STATE_POWERED_ON;
                break;
            default:
                break;
        }
    } else {
        switch(espwake->state) {
            case ESPWAKE_STATE_POWERED_ON:
            case ESPWAKE_STATE_POWERING_OFF:
                espwake->state = ESPWAKE_STATE_POWERED_OFF;
                break;
            default:
                break;
        }
    }
}

void espwake_init(espwake_t *espwake, espwake_if_t *interface) {
    espwake->interface = interface;
    espwake->seq_ready_semaphore = xSemaphoreCreateCounting(1, 0);
    espwake->seq_empty_semaphore = xSemaphoreCreateCounting(1, 1);
    espwake->sequence = ESPWAKE_SWITCH_SEQUENCE_NONE;
    espwake->state = espwake_if_poll_power_status(interface) ? ESPWAKE_STATE_POWERED_ON : ESPWAKE_STATE_POWERED_OFF;
    espwake_if_set_pwr_callback(interface, pwroff_irq_handler, espwake);
    xTaskCreate(espwake_switch_control_task, "espwake_switch_controller", 256, espwake, 2, &espwake->switch_controller_task);
}

void espwake_destroy(espwake_t *espwake) {
    vTaskDelete(espwake->switch_controller_task);
    vSemaphoreDelete(espwake->seq_ready_semaphore);
}

void espwake_power_on(espwake_t *espwake) {
    if (espwake->state == ESPWAKE_STATE_POWERED_ON || espwake->state == ESPWAKE_STATE_POWERING_ON) {
        return;
    }
    espwake->state = ESPWAKE_STATE_POWERING_ON;
    espwake_dispatch_sequence(espwake, ESPWAKE_SWITCH_SEQUENCE_START);
}

void espwake_shutdown(espwake_t *espwake) {
    if (espwake->state == ESPWAKE_STATE_POWERING_OFF || espwake->state == ESPWAKE_STATE_POWERED_OFF) {
        return;
    }
    espwake->state = ESPWAKE_STATE_POWERING_OFF;
    espwake_dispatch_sequence(espwake, ESPWAKE_SWITCH_SEQUENCE_SHUTDOWN);
}

void espwake_force_off(espwake_t *espwake) {
    if (espwake->state == ESPWAKE_STATE_POWERED_OFF) {
        return;
    }
    espwake->state = ESPWAKE_STATE_POWERING_OFF;
    espwake_dispatch_sequence(espwake, ESPWAKE_SWITCH_SEQUENCE_FORCEOFF);
}
