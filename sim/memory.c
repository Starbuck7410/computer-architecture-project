#include "memory.h"
#include "bus.h"


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


bool write_word_to_cache(Core * core, int address, uint32_t data){
    // This function assumes there is a cache hit, and 
    // checks for the MESI state of the block for the Shared 
    // state. If it is Shared, this function must alert the bus 
    // to make this block invalid in the other cores. (Rather, 
    // the bus 'snoops' and finds out about this on it's own,
    // but the implementation details do not matter as much)

    // This function returns whether we should stall the memory
    // stage because of a failed write to the cache. If it wasn't,
    // the processor must send a BUS_RD request to the bus, 
    // and stall until the bus finishes the request, and then
    // write to the cache block.

    uint32_t index = (address >> 3) & 0x3F; // Bits 8:3
    uint32_t offset = address & 0x7; // Bits 2:0
    TSRAM_Line* t_line = &core->cache.tsram[index];
    DSRAM_Line* d_line = &core->cache.dsram[index];

    switch (t_line->mesi_state){
        case MESI_EXCLUSIVE:
        case MESI_MODIFIED:
            d_line->word[offset] = data;
            t_line->mesi_state = MESI_MODIFIED;
            return true;
        case MESI_SHARED:
            send_bus_read_request(core, address, true);
            t_line->mesi_state = MESI_MODIFIED;
            return true;
        case MESI_INVALID:
            send_bus_read_request(core, address, false); // We gotta rethink this one
            return false;
    }
}


    




