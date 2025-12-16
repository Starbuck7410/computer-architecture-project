#include "memory.h"

extern SystemBus system_bus; // Guys this is for you.
// This means system_bus is a global variable, and exists EVERYWHERE



bool is_cache_hit(Cache * cache, int address){
    // Checks for cache hit using the tag and also the TSRAM state
    // If it is invalid, the cache is, well, invalid 
    // and is considered a miss.
}

uint32_t read_word_from_cache(Cache * cache, int address){
    // This function assumes there is a cache hit!
    // It should simply return the word from the cache.
}


bool write_word_to_cache(Cache * cache, int address, uint32_t data){
    // This function assumes there is a cache hit, and 
    // checks for the MESI state of the block for the Shared 
    // state. If it is Shared, this function must alert the bus 
    // to make this block invalid in the other cores. (Rather, 
    // the bus 'snoops' and finds out about this on it's own,
    // but the implementation details do not matter as much)
}


