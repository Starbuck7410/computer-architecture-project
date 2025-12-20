#include "general_utils.h"

bool send_bus_read_request(Core* core, uint32_t address, bool exclusive);
void invalidate_cache_block(Core* core[CORE_COUNT], int block_index, int safe_core_index);