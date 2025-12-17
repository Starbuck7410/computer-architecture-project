#include "memory.h"

extern SystemBus system_bus; // Guys this is for you.
// This means system_bus is a global variable, and exists EVERYWHERE



bool is_cache_hit(Cache * cache, int address){
    uint32_t index = (address >> 3) & 0x3F; // Bits 8:3
    uint32_t tag = (address >> 9) & 0xFFF; // Bits 20:9
    TSRAM_Line* line = &cache->tsram[index];

    if (line->tag == tag && line->mesi_state != MESI_INVALID) { // Cache hit
        return true;
    }
    return false; // Cache miss
}

uint32_t read_word_from_cache(Cache * cache, int address){
    // This function assumes there is a cache hit!
    // It should simply return the word from the cache.
    uint32_t index = (address >> 3) & 0x3F; // Bits 8:3
    uint32_t offset = address & 0x7; // Bits 2:0

    return cache->dsram[index].word[offset];
}


bool write_word_to_cache(Cache * cache, int address, uint32_t data, int core_id){
    // This function assumes there is a cache hit, and 
    // checks for the MESI state of the block for the Shared 
    // state. If it is Shared, this function must alert the bus 
    // to make this block invalid in the other cores. (Rather, 
    // the bus 'snoops' and finds out about this on it's own,
    // but the implementation details do not matter as much)
    uint32_t index = (address >> 3) & 0x3F; // Bits 8:3
    uint32_t offset = address & 0x7; // Bits 2:0
    TSRAM_Line* t_line = &cache->tsram[index];
    DSRAM_Line* d_line = &cache->dsram[index];

    if (t_line->mesi_state == MESI_EXCLUSIVE || t_line->mesi_state == MESI_MODIFIED) { // We can write immediately
        d_line->word[offset] = data;
        t_line->mesi_state = MESI_MODIFIED;
        return true;
    }
    else if (t_line->mesi_state == MESI_SHARED) { // Shared - the pipeline must stall
        if (system_bus.bus_cmd == BUS_NOCMD || system_bus.bus_orig_id == core_id) { 
            system_bus.bus_cmd = BUS_RDX;
            system_bus.bus_addr = address;
            system_bus.bus_orig_id = core_id;
        }
        return false;
    }
    return false;
}


