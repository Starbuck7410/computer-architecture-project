#include "memory.h"
#include "bus.h"

bool is_cache_hit(Cache * cache, int address){
    uint32_t index = (address >> 3) & 0x3F; // 6 bits
    uint32_t tag = (address >> 9) & 0xFFF; 
    
    if (cache->tsram[index].mesi_state != MESI_INVALID && cache->tsram[index].tag == tag) {
        return true;
    }
    return false;
}

uint32_t read_word_from_cache(Cache * cache, int address){
    uint32_t index = (address >> 3) & 0x3F; 
    uint32_t offset = address & 0x7; 
    return cache->dsram[index].word[offset];
}

bool write_word_to_cache(Core * core, int address, uint32_t data){
    uint32_t index = (address >> 3) & 0x3F;
    uint32_t offset = address & 0x7;
    uint32_t tag = (address >> 9) & 0xFFF;
    
    TSRAM_Line* t_line = &core->cache.tsram[index];
    DSRAM_Line* d_line = &core->cache.dsram[index];

    // Check Tag match first
    if (t_line->tag != tag || t_line->mesi_state == MESI_INVALID) {
        // Miss (Read for Ownership needed)
        send_bus_read_request(core, address, true);
        return false; 
    }

    // It's a Hit, check State
    switch (t_line->mesi_state){
        case MESI_MODIFIED:
        case MESI_EXCLUSIVE:
            d_line->word[offset] = data;
            t_line->mesi_state = MESI_MODIFIED;
            return true;
        case MESI_SHARED:
            // Need to upgrade to Exclusive (Bus Upgrade/Invalidate others)
            // For simplicity, we send a RDX.
            send_bus_read_request(core, address, true);
            return false; // STALL until bus done
        default:
            return false;
    }
}