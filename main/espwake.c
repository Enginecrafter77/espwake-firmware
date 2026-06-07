#include "espwake.h"
#include "espwake_if.h"

#include <time.h>

#define TIME_MS(N) (N / portTICK_RATE_MS)

#define MIN_HOLD_TIME_MS 2000

static inline uint32_t esp_system_time_millis() {
    static struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (time.tv_sec * 1000) + (time.tv_nsec / 1000000);
}

const char* espwake_state_name(espwake_state_t state) {
    switch(state) {
        case ESPWAKE_STATE_POWERED_OFF:
            return "power_off";
        case ESPWAKE_STATE_POWERING_OFF:
            return "powering_off";
        case ESPWAKE_STATE_SUSPENDED:
            return "suspended";
        case ESPWAKE_STATE_POWERING_ON:
            return "powering_on";
        case ESPWAKE_STATE_POWERED_ON:
            return "powered_on";
    }
    return "<<unknown>>";
}

const char* espwake_detected_state_name(espwake_detected_state_t state) {
    switch(state) {
        case ESPWAKE_DSTATE_POWER_OFF:
            return "power_off";
        case ESPWAKE_DSTATE_RUNNING:
            return "running";
        case ESPWAKE_DSTATE_SUSPENDED:
            return "suspended";
    }
    return "<<unknown>>";
}

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
    espwake_state_detector_t *detector = (espwake_state_detector_t*)arg;
    uint32_t current_time = esp_system_time_millis();
    // debounce -- nothing blinks this fast ...
    if ((current_time - detector->last_edge_time) > 25) {
        detector->prev_edge_time = detector->last_edge_time;
        detector->last_edge_time = current_time;
    }
    // ... but still save the power level to the last definite level
    detector->last_edge_signal = poweron;
}

static void espwake_state_sense_timer(void *arg) {
    TimerHandle_t timer = (TimerHandle_t)arg;
    espwake_state_detector_t *detector = (espwake_state_detector_t*)pvTimerGetTimerID(timer);

    uint32_t edge_pitch = detector->last_edge_time - detector->prev_edge_time;
    uint32_t hold_time = esp_system_time_millis() - detector->last_edge_time;
    espwake_detected_state_t new_state;
    if (edge_pitch < MIN_HOLD_TIME_MS && hold_time < MIN_HOLD_TIME_MS) {
        // two edges in short succession => power led is blinking => assume ACPI S3
        new_state = ESPWAKE_DSTATE_SUSPENDED;
    } else {
        new_state = detector->last_edge_signal ? ESPWAKE_DSTATE_RUNNING : ESPWAKE_DSTATE_POWER_OFF;
    }
    
    if (new_state != detector->state) {
        detector->state = new_state;
        if (detector->callback != NULL) {
            detector->callback(new_state, detector->callback_arg);
        }
    }
}

static void espwake_on_detected_state_change(espwake_detected_state_t new_state, void *arg) {
    espwake_t *espwake = (espwake_t*)arg;
    switch (new_state) {
        case ESPWAKE_DSTATE_RUNNING:
            espwake->state = ESPWAKE_STATE_POWERED_ON;
            break;
        case ESPWAKE_DSTATE_POWER_OFF:
            espwake->state = ESPWAKE_STATE_POWERED_OFF;
            break;
        case ESPWAKE_DSTATE_SUSPENDED:
            espwake->state = ESPWAKE_STATE_SUSPENDED;
            break;
    }
}

void espwake_state_detector_init(espwake_state_detector_t *detector, espwake_if_t *interface) {
    detector->interface = interface;
    detector->last_edge_signal = espwake_if_poll_power_status(interface);
    detector->last_edge_time = esp_system_time_millis();
    detector->prev_edge_time = 0;
    detector->state = espwake_if_poll_power_status(interface) ? ESPWAKE_DSTATE_RUNNING : ESPWAKE_DSTATE_POWER_OFF;
    detector->callback = NULL;
    detector->callback_arg = NULL;

    espwake_if_set_pwr_callback(interface, pwroff_irq_handler, detector);
    detector->eval_timer = xTimerCreate("espwake_state_sense", TIME_MS(5000), pdTRUE, detector, espwake_state_sense_timer);
    xTimerStart(detector->eval_timer, 0);
}

void espwake_state_detector_destroy(espwake_state_detector_t *detector) {
    xTimerDelete(detector->eval_timer, 0);
}

void espwake_init(espwake_t *espwake, espwake_if_t *interface) {
    espwake->interface = interface;
    espwake->seq_ready_semaphore = xSemaphoreCreateCounting(1, 0);
    espwake->seq_empty_semaphore = xSemaphoreCreateCounting(1, 1);
    espwake->sequence = ESPWAKE_SWITCH_SEQUENCE_NONE;
    espwake->state = espwake_if_poll_power_status(interface) ? ESPWAKE_STATE_POWERED_ON : ESPWAKE_STATE_POWERED_OFF;
    espwake_state_detector_init(&espwake->detector, interface);
    espwake_state_detector_set_callback(&espwake->detector, espwake_on_detected_state_change, espwake);
    xTaskCreate(espwake_switch_control_task, "espwake_switch_controller", 256, espwake, 2, &espwake->switch_controller_task);
}

void espwake_destroy(espwake_t *espwake) {
    espwake_state_detector_destroy(&espwake->detector);
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
    if (espwake->state == ESPWAKE_STATE_POWERING_OFF || espwake->state == ESPWAKE_STATE_POWERED_OFF || espwake->state == ESPWAKE_STATE_SUSPENDED) {
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
