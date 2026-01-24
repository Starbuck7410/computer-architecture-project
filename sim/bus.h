#pragma once
#include "general-utils.h"

extern bus_T system_bus;

void send_bus_read_request(core_T* core, uint32_t address, bool exclusive);
void init_bus(core_T * core[CORE_COUNT]);
void bus_handler();