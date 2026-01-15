#include "general_utils.h"

bool is_cache_hit(Cache* cache, int address);
uint32_t read_word_from_cache(Cache* cache, int address);
bool write_word_to_cache(Core * core, int address, uint32_t data);
