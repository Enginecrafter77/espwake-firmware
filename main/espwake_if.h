#pragma once

#include <stdbool.h>

void espwake_if_init();
void espwake_if_set_switch(bool switch_value);
bool espwake_if_poll_power_status();
