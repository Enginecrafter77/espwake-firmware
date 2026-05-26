#pragma once

#include <stdbool.h>

typedef void (*espwake_if_switch_setter_t)(bool state);
typedef bool (*espwake_if_power_poller_t)();
typedef void (*espwake_if_power_callback_t)(bool new_state, void *user_arg);

typedef struct {
    espwake_if_switch_setter_t switch_setter;
    espwake_if_power_poller_t power_poller;
    espwake_if_power_callback_t power_callback;
    void *power_callback_arg;
} espwake_if_t;

void espwake_if_init(espwake_if_t *interface);

static inline void espwake_if_set_switch(espwake_if_t *interface, bool switch_value) { interface->switch_setter(switch_value); }
static inline void espwake_if_set_pwr_callback(espwake_if_t *interface, espwake_if_power_callback_t callback, void *user_arg) { interface->power_callback = callback; interface->power_callback_arg = user_arg; }
static inline bool espwake_if_poll_power_status(espwake_if_t *interface) { return interface->power_poller(); }
