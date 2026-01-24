#include "general-utils.h"

bool is_cache_hit(cache_T * cache, int address);
uint32_t read_word_from_cache(cache_T * cache, int address);
bool write_word_to_cache(core_T * core, int address, uint32_t data);
