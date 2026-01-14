#pragma once
#include "general_utils.h"

extern SystemBus system_bus;

void send_bus_read_request(Core* core, uint32_t address, bool exclusive);
void init_bus(Core * core[CORE_COUNT]);
void bus_handler();