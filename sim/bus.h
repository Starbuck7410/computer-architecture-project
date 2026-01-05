#include "general_utils.h"

extern SystemBus system_bus; // Guys this is for you.
// This means system_bus is a global variable, and exists EVERYWHERE

void send_bus_read_request(Core* core, uint32_t address, bool exclusive);
//void invalidate_cache_block(int block_index, int safe_core_index);
void bus_request_handler();
void bus_handler();